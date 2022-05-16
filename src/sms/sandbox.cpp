#include <iostream>
#include <sstream>
#include <vector>



void write(std::ostream& out,
           const std::vector<double>& x) {
  uint64_t N = x.size();
  out.write(reinterpret_cast<const char*>(&N), sizeof(N));
  out.write(reinterpret_cast<const char*>(x.data()), N * sizeof(double));
}

void read(std::istream& in,
          std::vector<double>& y) {
  uint64_t N;
  in.read(reinterpret_cast<char*>(&N), sizeof(N));
  y.resize(N);
  in.read(reinterpret_cast<char*>(y.data()), N * sizeof(double));
}


int main() {
  using v_t = std::vector<double>;
  v_t x{1.2, 3.9, 7.3};

  std::stringstream out;
  write(out, x);

  std::stringstream in(out.str());
  std::vector<double> y;
  read(in, y);

  for (int n = 0; n < y.size(); ++n)
    std::cout << "y[" << n << "] = " << y[n] << std::endl;
}
