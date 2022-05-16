#include <stan_model_server/server.hpp>
#include <fstream>
#include <iostream>
#include <string>

// CALL AS: <exe> <json-data-file> <seed>
// argc == 3;  argv[1] == json data file; argv[2] == seed
int main(int argc, const char* argv[]) {
  try {
    const char* json_data_file(argv[1]);
    std::fstream in(json_data_file, std::fstream::in);
    cmdstan::json::json_data data_vc(in);

    unsigned int seed = std::stoul(argv[2], nullptr, 0);

    stan::model::model_base& model = new_model(data_vc, seed, &std::cout);

    stan_model_server::server server(model);
    server.serve();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}

    // std::shared_ptr<stan::io::var_context> vc_ptr
    // = std::make_shared<cmdstan::json::json_data>(jd_vc);
    // auto model = new_model(*vc_ptr, seed, &std::cout);
