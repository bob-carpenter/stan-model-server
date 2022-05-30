#include <cmdstan/io/json/json_data.hpp>
#include <stan/math.hpp>
#include <stan/io/empty_var_context.hpp>
#include <stan/model/model_base.hpp>

#include <CLI11/CLI11.hpp>

#include <fstream>
#include <iostream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/**
 * Allocate and return a new model as a reference given the specified
 * data context, seed, and message stream.  This function is defined
 * in the generated model class.
 *
 * @param[in] data_context context for reading model data
 * @param[in] seed random seed for transformed data block
 * @param[in] msg_stream stream to which to send messages printed by the model
 */
stan::model::model_base& new_model(stan::io::var_context &data_context,
                                   unsigned int seed, std::ostream *msg_stream);


/**
 * Turn off synchronization within standard streams and don't flush
 * `std::out` before reading `std::in`.  Together, these should
 * substantially speed up I/O without violating any contracts of the
 * server.
 */
void un_synch_un_autoflush_std_io_for_speed() {
  // remove synch on std I/O
  std::ios_base::sync_with_stdio(false);
  // don't flush std::out before read std::cin
  std::cin.tie(NULL);
}

/**
 * Functor for a model of the specified template type and its log
 * density configuration in terms of dropping constants and/or the
 * change-of-variables adjustment.
 *
 * @tparam M type of model
 */
template <class M>
struct model_functor {
  /** Stan model */
  const M& model_;

  /** `true` if including constant terms */
  const bool propto_;

  /** `true` if including change-of-variables terms */
  const bool jacobian_;

  /** Output stream for messages from Stan model */
  std::ostream& out_;

  /**
   * Construct a model functor from the specified model, output
   * stream, and specification of whether constants should be dropped
   * and whether the change-of-variables terms should be dropped.
   *
   * @param[in] m Stan model
   * @param[in] propto `true` if log density drops constant terms
   * @param[in] jacobian `true` if log density includes change-of-variables
   * terms
   * @param[in] out output stream for messages from model
   */
  model_functor(const M& m, bool propto, bool jacobian, std::ostream& out)
      : model_(m), propto_(propto), jacobian_(jacobian), out_(out) { }

  /**
   * Return the log density for the specified unconstrained
   * parameters, including normalizing terms and change-of-variables
   * terms as specified in the constructor.
   *
   * @tparam T real scalar type for the arguments and return
   * @param theta unconstrained parameters
   * @throw std::exception if model throws exception evaluating log density
   */
  template <typename T>
  T operator()(const Eigen::Matrix<T, Eigen::Dynamic, 1>& theta) const {
    // const cast is safe---theta not modified
    auto params_r = const_cast<Eigen::Matrix<T, Eigen::Dynamic, 1>&>(theta);
    return propto_
        ? (jacobian_
           ? model_.template log_prob<true, true, T>(params_r, &out_)
           : model_.template log_prob<true, false, T>(params_r, &out_))
        : (jacobian_
           ? model_.template log_prob<false, true, T>(params_r, &out_)
           : model_.template log_prob<false, false, T>(params_r, &out_));
  }
};

/**
 * Return an appropriately typed model functor from the specified model, given
 * the specified output stream and flags indicating whether to drop  constant
 * terms and include change-of-variables terms.  Unlike the `model_functor` constructor,
 * this factory function provides type inference for `M`.
 *
 * @tparam M type of Stan model
 * @param[in] m Stan model
 * @param[in] propto `true` if log density drops constant terms
 * @param[in] jacobian `true` if log density includes change-of-variables
 * terms
 * @param[in] out output stream for messages from model
 */
template <typename M>
model_functor<M> create_model_functor(const M& m, bool propto, bool jacobian,
                                       std::ostream& out) {
  return model_functor<M>(m, propto, jacobian, out);
}


/**
 * Class for managing the server read-evaluate-print loop (REPL).
 * Holds a reference to the model (its memory is managed by the config
 * object), the base pseudo-RNG reused through the server, and the input,
 * output, and error stream to use.
 *
 * Standard server operation reads from the input stream, writes to
 * the output stream, and sends errors and messages from Stan programs
 * to the error stream.
 */
struct repl {
  boost::ecuyer1988 base_rng_;
  stan::model::model_base& model_;
  std::istream& in_;
  std::ostream& out_;
  std::ostream& err_;

  /**
   * Construct a REPL with a base model, pseudo-RNG seed, input
   * stream, output stream, and error stream.  The output and error
   * stream use double-precision for printing floatoing-point numbers.
   *
   * @param[in] model Stan model
   * @param[in] seed seed for pseudo-RNG
   * @param[in] in input stream
   * @param[in] out output stream
   * @param[in] err error stream
   */
  repl(stan::model::model_base& model, uint seed,
       std::istream& in, std::ostream& out, std::ostream& err)
      : base_rng_(seed),
        model_(model),
        in_(in), out_(out), err_(err) {
    base_rng_.discard(1000000000000L);
    out_ << std::setprecision(std::numeric_limits<double>::digits10);
    err_ << std::setprecision(std::numeric_limits<double>::digits10);
  }

  /**
   * Execute the read-eval-print loop until it returns `false` or
   * throws an uncaught exception.
   */
  void loop() {
    while (read_eval_print());
  }

  /**
   * Write elements of the container to the output stream separated by
   * commas.
   *
   * @tparam T container type
   * @param[in] x container
   */
  template <typename T>
  void write_csv(T&& x) {
    for (size_t i = 0; i < x.size(); ++i) {
      if (i > 0) out_ << ',';
      out_ << x[i];
    }
  }

  /**
   * Write elements of the Eigen container to the output stream
   * separated by commas.  Matrices will be dumped in column-major
   * order.
   *
   * @tparam T type of container
   * @param[in] x container
   */
  template <typename T>
  void write_csv_eigen(T&& x) {
    for (int i = 0; i < x.size(); ++i) {
      if (i > 0) out_ << ',';
      out_ << x(i);
    }
  }

  /**
   * Return the number of unconstrained parameters.
   *
   * @return number of unconstrained parameters
   */
  int get_num_unc_params() {
    bool include_generated_quantities = false;
    bool include_transformed_parameters = false;
    std::vector<std::string> names;
    model_.unconstrained_param_names(names, include_generated_quantities,
                                     include_transformed_parameters);
    return names.size();
  }

  /**
   * Execute the read-eval-print loop until the `quit` instruction is executed
   * or until an uncaught exception is thrown.
   *
   * @return `true` if it should be called again and `false` to exit
   */
  bool read_eval_print() {
    std::string line;
    std::getline(in_, line);
    std::stringstream cmd(line);
    std::string instruction;
    cmd >> instruction;
    try {
      if (instruction == "quit")
	return quit();
      if (instruction == "name")
	return name();
      if (instruction == "param_names")
	return param_names(cmd);
      if (instruction == "param_unc_names")
	return param_unc_names();
      if (instruction == "param_num")
	return param_num(cmd);
      if (instruction == "param_unc_num")
	return param_unc_num();
      if (instruction == "param_constrain")
	return param_constrain(cmd);
      if (instruction == "param_unconstrain")
	return param_unconstrain(cmd);
      if (instruction == "log_density")
	return log_density(cmd);
      out_ << "UNKNOWN" << std::endl;
      err_ << "Unknown instruction: " << instruction << std::endl;
    } catch (const std::exception& e) {
      out_ << "ERROR" << std::endl;
      err_ << "Error in instruction: " << instruction << ".  "
	   << "Error message: " << e.what() << std::endl;
    }
    return true;
  }

  // REPL INSTRUCTIONS START HERE

  /**
   * Print quit message to output stream and return `false`.
   *
   * @return `false`
   */
  bool quit() {
    out_ << "REPL quit." << std::endl;
    return false;
  }

  /**
   * Print the model name to output stream and return `true`.
   *
   * @return `true`
   */
  bool name() {
    out_ << model_.model_name() << std::endl;
    return true;
  }

  /**
   * Read whether or not to include transformed parameters and include
   * generated quantities from the specified command stream, write
   * the relevant constrained parameter names to the output stream,
   * and return `true`.
   *
   * @param cmd command input stream
   * @return `true`
   */
  bool param_names(std::istream& cmd) {
    bool include_transformed_parameters;
    cmd >> include_transformed_parameters;
    bool include_generated_quantities;
    cmd >> include_generated_quantities;
    std::vector<std::string> names;
    model_.constrained_param_names(names,
                                   include_transformed_parameters,
                                   include_generated_quantities);
    write_csv(names);
    out_ << std::endl;
    return true;
  }

  /**
   * Write the unconstrained parameter names to the output stream and
   * return `true`.  The return excludes transformed parameters and
   * generated quantities, which do not have unconstrained forms.  .
   *
   * @return `true`
   */
  bool param_unc_names() {
     std::vector<std::string> names;
     constexpr static bool include_transformed_parameters = false;
     constexpr static bool include_generated_quantities = false;
     model_.unconstrained_param_names(names, include_transformed_parameters,
                                      include_generated_quantities);
     write_csv(names);
     out_ << std::endl;
     return true;
  }

  /**
   * Read whether to include transformed parameters and whether to
   * include generated quantities from the input stream, write
   * the relevant number of parameters to the output stream, and
   * return `true`.
   *
   * @param cmd command input stream
   * @return `false`
   */
  bool param_num(std::istream& cmd) {
    bool include_transformed_parameters;
    cmd >> include_transformed_parameters;
    bool include_generated_quantities;
    cmd >> include_generated_quantities;
    std::vector<std::string> names;
    model_.constrained_param_names(names,
                                   include_transformed_parameters,
                                   include_generated_quantities);
    out_ << names.size() << std::endl;
    return true;
  }

  /**
   * Write the number of unconstrained parameters to the output stream
   * and return `true`.  This is just the parameters because
   * transformed parameters and generated quantities do not have
   * unconstrained forms.
   *
   * @return `true`
   */
  bool param_unc_num() {
    std::vector<std::string> names;
    bool include_transformed_parameters = false;
    bool include_generated_quantities = false;
    model_.unconstrained_param_names(names,
                                     include_transformed_parameters,
                                     include_generated_quantities);
    out_ << names.size() << std::endl;
    return true;
  }

  /**
   * Read whether to include transformed parameters, whether to
   * include generated quantities, and the unconstrained parameters
   * from the intput stream, then write the relevant constrained
   * parameters to the output stream, and return `true`.
   *
   * @param[in] cmd command input stream
   * @return `true`
   */
  bool param_constrain(std::istream& cmd) {
    bool include_transformed_parameters;
    cmd >> include_transformed_parameters;
    bool include_generated_quantities;
    cmd >> include_generated_quantities;
    Eigen::VectorXd params_unc(get_num_unc_params());
    for (int n = 0; n < params_unc.size(); ++n)
      cmd >> params_unc(n);
    Eigen::VectorXd params;
    model_.write_array(base_rng_, params_unc, params,
                       include_transformed_parameters,
                       include_generated_quantities, &err_);
    write_csv_eigen(params);
    out_ << std::endl;
    return true;
  }

  /**
   * Read the constrained parameters from the input stream, write the
   * unconstrained parameters to the output stream, and return `true`.
   * This only includes the parameters, not the transformed parameters
   * or generated quantities, which do not have unconstrained forms.
   *
   * @param[in] cmd command input stream
   * @return `true`
   */
  bool param_unconstrain(std::istream& cmd) {
    std::string line;
    std::getline(cmd, line);
    std::stringstream in(line);
    cmdstan::json::json_data inits_context(in);
    Eigen::VectorXd params_unc;
    model_.transform_inits(inits_context, params_unc, &err_);
    write_csv_eigen(params_unc);
    out_ << std::endl;
    return true;
  }

  /**
   * Read whether to exclude constants, whether to include
   * change-of-variables adjustments, whether to include the gradient,
   * and whether to include the Jacobian, and the unconstrained
   * parameters, then write the log density and gradient or Hessian if
   * specified, and return `true`.
   *
   * The gradients are computed with automatic differentiation and the
   * Hessian by finite differences over the autodiff gradients.
   *
   * @param[in] cmd command input stream
   * @return `true`
   */
  bool log_density(std::istream& cmd) {
    bool propto = true;
    cmd >> propto;
    bool jacobian = true;
    cmd >> jacobian;
    bool include_grad = true;
    cmd >> include_grad;
    bool include_hessian = true;
    cmd >> include_hessian;

    auto model_functor = create_model_functor(model_, propto, jacobian, err_);
    Eigen::VectorXd params_unc(get_num_unc_params());
    for (int n = 0; n < params_unc.size(); ++n)
      cmd >> params_unc(n);
    double log_density;
    Eigen::VectorXd grad;
    Eigen::MatrixXd hess;
    if (include_hessian) {
      stan::math::internal::finite_diff_hessian_auto(model_functor,
          params_unc, log_density, grad, hess);
    } else {
      stan::math::gradient(model_functor, params_unc, log_density, grad);
    }
    out_ << log_density;
    if (include_grad) {
      out_ << ",";
      write_csv_eigen(grad);
    }
    if (include_hessian) {
      out_ << ",";
      write_csv_eigen(hess);  // column major output
    }
    out_ << std::endl;
    return true;
  }
};  // struct repl

/**
 * Object managing server configuration.
 */
struct config {
  /**
   * Path to data file.
   */
  std::string data_file_path_;

  /**
   * Random seed used to construct server.
   */
  unsigned int seed_;

  /**
   * Pointer to Stan model of base class.
   */
  stan::model::model_base* model_;

  /**
   * Construct model based on command-line arguments.  Holds a pointer
   * to the Stan model class and manages its resources following the
   * RAII pattern (allocate resources in constructor, free in
   * destructor).
   *
   * @param[in] argc number of command-line arguments (including executable)
   * @param[in] argv command-line arguments in C string format
   * @throw std::exception if there is an error reading data from the
   * specified file
   */
  config(int argc, const char* argv[]) :
      data_file_path_(), seed_(1234) {
    parse(argc, argv);
    create_model();
  }

  /**
   * Free the model's memory.
   */
  ~config() { delete model_; }

  /**
   * Parse the command-line arguments and set the data file path and
   * seed for this class.
   *
   * @param[in] argc number of command-line arguments (including executable)
   * @param[in] argv command-line arguments in C string format
   */
  int parse(int argc, const char* argv[]) {
    CLI::App app{"Stan Command Line Interface"};
    app.add_option("-d, --data", data_file_path_,
                   "File containing data in JSON", true)
        -> check(CLI::ExistingFile);
    app.add_option("-s, --seed", seed_,
                   "Random seed", true)
        -> check(CLI::PositiveNumber);
    CLI11_PARSE(app, argc, argv);
    return 0;
  }

  /**
   * Allocate model and initialize data and transformed data.  Use the
   * JSON-formatted data at the data file path, or an empty context if
   * no path was given.
   *
   * @throw std::runtime_error if there is an error reading the file
   */
  void create_model() {
    if (data_file_path_ == "") {
      stan::io::empty_var_context empty_data;
      model_ = &new_model(empty_data, seed_, &std::cerr);
      return;
    }
    std::ifstream in(data_file_path_);
    if (!in.good())
      throw std::runtime_error("Cannot read input file: " + data_file_path_);
    cmdstan::json::json_data data(in);
    in.close();
    model_ = &new_model(data, seed_, &std::cerr);
  }
};


/**
 * Setup the server based on the command-line arguments and run its
 * REPL loop until clean exit or exceptional exit.
 *
 * @param[in] argc number of command-line arguments (including executable)
 * @param[in] argv command-line arguments in C string format
 * @return return code (0 is standard shut down, non-0 is non-standard exit)
 */
int main(int argc, const char* argv[]) {
  constexpr static int SUCCESS_RC = 0;
  constexpr static int STD_EXCEPT_RC = 5001;
  constexpr static int UNKNOWN_EXCEPT_RC = 5002;
  try {
    un_synch_un_autoflush_std_io_for_speed();
    config cfg(argc, argv);
    repl r(*cfg.model_, cfg.seed_, std::cin, std::cout, std::cerr);
    r.loop();
    return SUCCESS_RC;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Could not construct REPL (std::exception): "
              << e.what() << std::endl;
    return STD_EXCEPT_RC;
  } catch (...) {
    std::cerr << "ERROR: Could not construct REPL (unknown exception)."
              << std::endl;
    return UNKNOWN_EXCEPT_RC;
  }
}
