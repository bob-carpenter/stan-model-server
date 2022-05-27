#include <stan/math.hpp>
#include <cmdstan/io/json/json_data.hpp>
#include <stan/io/empty_var_context.hpp>
#include <stan/model/model_base.hpp>
#include <fstream>
#include <iostream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <CLI11/CLI11.hpp>

stan::model::model_base& new_model(stan::io::var_context &data_context,
                                   unsigned int seed, std::ostream *msg_stream);

template <class M>
struct model_functor {
  const M& model_;
  const bool propto_;
  const bool jacobian_;
  std::ostream& out_;

  model_functor(const M& m, bool propto, bool jacobian, std::ostream& out)
      : model_(m), propto_(propto), jacobian_(jacobian), out_(out) { }

  template <typename T>
  T operator()(const Eigen::Matrix<T, Eigen::Dynamic, 1>& x) const {
    auto params_r = const_cast<Eigen::Matrix<T, Eigen::Dynamic, 1>&>(x);
    return propto_
        ? (jacobian_
           ? model_.template log_prob<true, true, T>(params_r, &out_)
           : model_.template log_prob<true, false, T>(params_r, &out_))
        : (jacobian_
           ? model_.template log_prob<false, true, T>(params_r, &out_)
           : model_.template log_prob<false, false, T>(params_r, &out_));
  }
};

// factory function to automate type inference for MM
template <typename MM>
model_functor<MM> create_model_functor(const MM& m, bool propto, bool jacobian,
                                       std::ostream& out) {
  return model_functor<MM>(m, propto, jacobian, out);
}



struct repl {
  boost::ecuyer1988 base_rng_;
  stan::model::model_base& model_;
  std::istream& in_;
  std::ostream& out_;
  std::ostream& err_;

  repl(stan::model::model_base& model, uint seed,
       std::istream& in, std::ostream& out, std::ostream& err)
      : base_rng_(seed),
        model_(model),
        in_(in), out_(out), err_(err) {
    base_rng_.discard(1000000000000L);
    out_ << std::setprecision(std::numeric_limits<double>::digits10);
    err_ << std::setprecision(std::numeric_limits<double>::digits10);
  }

  int loop() {
    while (read_eval_print());
    return 0;
  }

  template <typename T>
  void write_csv(T&& x) {
    for (size_t i = 0; i < x.size(); ++i) {
      if (i > 0) out_ << ',';
      out_ << x[i];
    }
  }

  template <typename T>
  void write_csv_eigen(T&& x) {
    for (int i = 0; i < x.size(); ++i) {
      if (i > 0) out_ << ',';
      out_ << x(i);
    }
  }

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
	return param_unc_names(cmd);
      if (instruction == "param_num")
	return param_num(cmd);
      if (instruction == "param_unc_num")
	return param_unc_num(cmd);
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

  bool quit() {
    out_ << "REPL quit." << std::endl;
    return false;
  }

  bool name() {
    out_ << model_.model_name() << std::endl;
    return true;
  }

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

  bool param_unc_names(std::istream& cmd) {
     bool include_transformed_parameters;
     cmd >> include_transformed_parameters;
     bool include_generated_quantities;
     cmd >> include_generated_quantities;
     std::vector<std::string> names;
     model_.unconstrained_param_names(names,
                                      include_transformed_parameters,
                                      include_generated_quantities);
     write_csv(names);
     out_ << std::endl;
     return true;
  }

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

  bool param_unc_num(std::istream& cmd) {
    bool include_transformed_parameters;
    cmd >> include_transformed_parameters;
    bool include_generated_quantities;
    cmd >> include_generated_quantities;
    std::vector<std::string> names;
    model_.unconstrained_param_names(names,
                                     include_transformed_parameters,
                                     include_generated_quantities);
    out_ << names.size() << std::endl;
    return true;
  }

  int get_num_params() {
    // TODO(carpenter): cache this straight off
    bool incl_gqs = false;
    bool incl_tps = false;
    std::vector<std::string> names;
    model_.unconstrained_param_names(names, incl_gqs, incl_tps);
    return names.size();
  }

  bool param_constrain(std::istream& cmd) {
    bool include_transformed_parameters;
    cmd >> include_transformed_parameters;
    bool include_generated_quantities;
    cmd >> include_generated_quantities;
    Eigen::VectorXd params_unc(get_num_params());
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

  bool param_unconstrain(std::istream& cmd) {
    std::string line;
    std::getline(cmd, line);
    std::stringstream in(line);
    cmdstan::json::json_data inits_context(in);
    Eigen::VectorXd params_unc(1);
    model_.transform_inits(inits_context, params_unc, &err_);
    write_csv_eigen(params_unc);
    out_ << std::endl;
    return true;
  }

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
    int N = get_num_params();
    Eigen::VectorXd params_unc(get_num_params());
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

void speedy_io() {
  // remove synch on std I/O
  std::ios_base::sync_with_stdio(false);
  // don't flush std::out before read std::cin
  std::cin.tie(NULL);
}

struct config {
  std::string data_file_path_;
  unsigned int seed_;
  stan::model::model_base* model_;

  config(int argc, const char* argv[]) :
      data_file_path_(), seed_(1234) {
    parse(argc, argv);
    create_model();
  }

  ~config() { delete model_; }

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

  void create_model() {
    if (data_file_path_ == "") {
      stan::io::empty_var_context empty_data;
      model_ = &new_model(empty_data, seed_, &std::cerr);
      return;
    }
    std::ifstream in(data_file_path_);
    if (!in.good())
      throw std::runtime_error("Cannot read input file.");
    cmdstan::json::json_data data(in);
    in.close();
    model_ = &new_model(data, seed_, &std::cerr);
  }
};


int main(int argc, const char* argv[]) {
  try {
    speedy_io();
    config cfg(argc, argv);
    repl r(*cfg.model_, cfg.seed_, std::cin, std::cout, std::cerr);
    return r.loop();
  } catch (const std::exception& e) {
    std::cerr << "Uncaught std::exception: " << e.what() << std::endl;
    return 5001;
  } catch (...) {
    std::cerr << "Uncaught exception of unknown type." << std::endl;
    return 5002;
  }
}
