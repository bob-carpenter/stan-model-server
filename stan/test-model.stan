parameters {
  vector[3] alpha;
}
model {
  alpha ~ normal(0, 1);
}
generated quantities {
  real alpha_bar = mean(alpha);
}
