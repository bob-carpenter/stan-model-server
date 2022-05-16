#ifndef STAN_MODEL_SERVER_SERVER_HPP
#define STAN_MODEL_SERVER_SERVER_HPP

#include <stan/model/model_base.hpp>

namespace stan_model_server {

class server {
  const stan::model::model_base& model_;
 public:
  server(const stan_model& model) : model_(model) { }
  void serve() {
    while (true) {
      serve_one();
    }
  }
  void serve_one() {

  }
};

} // namespace stan_model_server

#endif
