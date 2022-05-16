#include <cmdstan/io/json/json_data.hpp>
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

using shared_context_ptr = std::shared_ptr<stan::io::var_context>;

int main(int argc, const char* argv[]) {

  // Command-line arguments
  std::string filename;
  CLI::App app{"Allowed options"};
  app.add_option("input_file", filename, "Input data files in JSON notation.", true)
      ->check(CLI::ExistingFile);

  try {
    CLI11_PARSE(app, argc, argv);
  } catch (const CLI::ParseError &e) {
    std::cout << e.get_exit_code();
    return app.exit(e);
  }

  std::ifstream infile;
  infile.open(filename.c_str());
  if (!infile.good()) {
      std::cout << "Cannot read input file: " << filename << "."
                << std::endl;
      return -1;
  }
  cmdstan::json::json_data var_context(infile);
  infile.close();
  shared_context_ptr json_context = std::make_shared<cmdstan::json::json_data>(var_context);

  stan::model::model_base& model = new_model(*json_context, 42u, &std::cout);
  std::cout << model.model_name() << std::endl;
}
