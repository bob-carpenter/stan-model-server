#include <cmdstan/io/json/json_data.hpp>
#include <stan/io/empty_var_context.hpp>
#include <stan/model/model_base.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <CLI11/CLI11.hpp>

stan::model::model_base& new_model(stan::io::var_context &data_context,
                                   unsigned int seed, std::ostream *msg_stream);

void param_unconstrain(stan::model::model_base& model,
                       std::istream& in, std::ostream& out,
                       std::ostream& err,
                       std::stringstream& cmd) {
  std::vector<std::string> names;
  static constexpr bool include_transformed_parameters = false;
  static constexpr bool include_generated_quantities = false;
  model.constrained_param_names(names, include_transformed_parameters,
                                include_generated_quantities);
  auto N = names.size();
  std::vector<double> params(N);
  std::vector<int> dummy_params_i;
  for (int n = 0; n < params.size(); ++n)
    cmd >> params[n];
  std::vector<double> params_unc;
  //  model.transform_inits_impl(params, dummy_params_i, params_unc,
  // std::cerr);
  for (int i = 0; i < params_unc.size(); ++i) {
    if (i > 0) out << ',';
    out << params_unc[i];
  }
}

template <typename RNG>
void param_constrain(stan::model::model_base& model,
                     std::istream& in, std::ostream& out,
                     std::ostream& err, RNG& base_rng,
                     std::stringstream& cmd) {
  std::vector<std::string> names;
  bool include_transformed_parameters;
  cmd >> include_transformed_parameters;
  bool include_generated_quantities;
  cmd >> include_generated_quantities;
  bool incl_gqs = false;
  model.unconstrained_param_names(names, include_generated_quantities,
                                  include_generated_quantities);
  auto N = names.size();
  Eigen::VectorXd params_unc(N);
  for (int n = 0; n < params_unc.size(); ++n)
    cmd >> params_unc(n);
  Eigen::VectorXd params;
  std::vector<int> dummy_params_i;
  model.write_array(base_rng, params_unc, params,
                    include_transformed_parameters,
                    include_generated_quantities, &err);

  for (int i = 0; i < params_unc.size(); ++i) {
    if (i > 0) out << ',';
    out << params[i];
  }
}

struct repl {
  boost::ecuyer1988 base_rng_;
  stan::model::model_base& model_;
  std::istream& in_;
  std::ostream& out_;
  std::ostream& err_;

  template <typename T>
  repl(T&& data_context, uint seed,
       std::istream& in, std::ostream& out, std::ostream& err)
      : base_rng_(seed),
        model_(new_model(data_context, seed, &err)),
        in_(in), out_(out), err_(err) {
    base_rng_.discard(1000000000000L);
  }

  ~repl() { delete &model_; }

  bool loop() {
    while (read_eval_print());
    return 0;  // 0 return code for normal termination
  }

  template <typename T>
  void write_csv(T&& x) {
    for (size_t i = 0; i < x.size(); ++i) {
      if (i > 0) out_ << ',';
      out_ << x[i];
    }
    out_ << std::endl;
  }

  bool read_eval_print() {
    std::string line;
    std::getline(in_, line);
    std::stringstream cmd(line);
    std::string instruction;
    cmd >> instruction;

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

    out_ << "Unknown instruction: " << instruction
         << std::endl;
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

  bool param_constrain(std::istream& cmd) {
    std::vector<std::string> names;
    bool include_transformed_parameters;
    cmd >> include_transformed_parameters;
    bool include_generated_quantities;
    cmd >> include_generated_quantities;
    bool incl_gqs = false;
    model_.unconstrained_param_names(names, include_generated_quantities,
                                     include_generated_quantities);
    auto N = names.size();
    Eigen::VectorXd params_unc(N);
    for (int n = 0; n < params_unc.size(); ++n)
      cmd >> params_unc(n);
    Eigen::VectorXd params;
    model_.write_array(base_rng_, params_unc, params,
                       include_transformed_parameters,
                       include_generated_quantities, &err_);
    write_csv(params);
    return true;
  }

  bool param_unconstrain(std::istream& cmd) {
    // TODO(carpenter): implement
    out_ << "param_unconstrain NOT IMPLEMENTED YET." << std::endl;
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

  config(int argc, const char* argv[]) :
      data_file_path_(), seed_(1234) {
    parse(argc, argv);
  }

  int parse(int argc, const char* argv[]) {
    CLI::App app{"Stan Command Line Interface"};
    app.add_option("-d, --data", data_file_path_,
                   "File containing data in JSON", true)
        -> check(CLI::ExistingFile);
    app.add_option("-s, --seed", seed_,
                   "Random seed", true)
        -> check(CLI::PositiveNumber);
    CLI11_PARSE(app, argc, argv);
  }
  cmdstan::io::var_context data() {
    if (cfg.data_file_path_ == "")
      return stan::io::empty_var_context();
    std::ifstream in(cfg.data_file_path_);
    if (!in.good())
      throw std::exception("Cannot read input file: " + cfg.data_file_path_);
    cmdstan::json::json_data context(in);
    in.close();
    return context;
  }
};


int main(int argc, const char* argv[]) {
  speedy_io();
  config cfg(argc, argv);
  auto d = cfg.data();
  repl r(data,  cfg.seed_,
         std::cin, std::cout, std::cerr);
  return r.loop();
}
