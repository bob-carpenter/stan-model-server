#include <cmdstan/io/json/json_data.hpp>
#include <stan/model/model_base.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

stan::model::model_base& new_model(stan::io::var_context &data_context,
                                   unsigned int seed, std::ostream *msg_stream);

using shared_context_ptr = std::shared_ptr<stan::io::var_context>;

int main(int argc, const char* argv[]) {
  std::fstream in("foo.json", std::fstream::in);
  cmdstan::json::json_data var_context(in);
  shared_context_ptr json_context = std::make_shared<cmdstan::json::json_data>(var_context);
  stan::model::model_base& model = new_model(*json_context, 42u, &std::cout);
  std::cout << model.model_name() << std::endl;
}
