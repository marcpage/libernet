#include "libernet/Trust.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__,        \
            #condition);                                                       \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 2;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    for (int i = 0; i < iterations; ++i) {
    	karma::Karma k0("0"), k1("1"), k1_1("1.1"), k1k("1000"), k1k_1("1000.1");
    	karma::Karma kismet("0.00000000000001"), k1_n_kismet(1,1);

		dotest(k0.getWholeKarma() == 0);
		dotest(k0.getKistem() == 0);
		dotest(k1.getWholeKarma() == 1);
		dotest(k1.getKistem() == 0);
		dotest(kismet.getWholeKarma() == 0);
		dotest(kismet.getKistem() == 1);
		dotest(k1_n_kismet.getWholeKarma() == 1);
		dotest(k1_n_kismet.getKistem() == 1);
		dotest(k1k.getWholeKarma() == 1000);
		dotest(k1k.getKistem() == 0);
		dotest(k1k_1.getWholeKarma() == 1000);
		dotest(k1k_1.getKistem() == 10000000000000);

		dotest(k0 * 10 == k0);
		dotest(k0 * 100 == k0);
		dotest(k1k / 1000 == k1);
		dotest(k0 + k1 == k1);
		dotest(k0 + k1k_1 == k1k_1);
		dotest(k1k + kismet == k1_n_kismet);
		dotest(k0.string() == "0");
		dotest(k1.string() == "1");
		dotest(k1_1.string() == "1.1");
		dotest(k1k.string() == "1000");
		dotest(k1k_1.string() == "1000.1");
		dotest(kismet.string() == "0.00000000000001");

		dotest(k0 < k1);
		dotest(k0 < k1_1);
		dotest(k0 < k1k);
		dotest(k0 < k1k_1);
		dotest(k0 < kismet);
		dotest(k0 < k1_n_kismet);

		dotest(k1 > k0);
		dotest(k1 < k1_1);
		dotest(k1 < k1k);
		dotest(k1 < k1k_1);
		dotest(k1 > kismet);
		dotest(k1 < k1_n_kismet);

		dotest(k1_1 > k1);
		dotest(k1_1 > k0);
		dotest(k1_1 < k1k);
		dotest(k1_1 < k1k_1);
		dotest(k1_1 > kismet);
		dotest(k1_1 < k1_n_kismet);

		dotest(k1k > k1);
		dotest(k1k > k0);
		dotest(k1k > k1_1);
		dotest(k1k < k1k_1);
		dotest(k1k > kismet);
		dotest(k1k > k1_n_kismet);

		dotest(k1k_1 > k1);
		dotest(k1k_1 > k0);
		dotest(k1k_1 > k1_1);
		dotest(k1k_1 < k1k);
		dotest(k1k_1 > kismet);
		dotest(k1k_1 > k1_n_kismet);

		dotest(kismet <= k1);
		dotest(kismet > k0);
		dotest(kismet < k1_1);
		dotest(kismet < k1k);
		dotest(kismet < k1k_1);
		dotest(kismet < k1_n_kismet);

		dotest(k1_n_kismet >= k1);
		dotest(k1_n_kismet > k0);
		dotest(k1_n_kismet > k1_1);
		dotest(k1_n_kismet > k1k);
		dotest(k1_n_kismet > k1k_1);
		dotest(k1_n_kismet > kismet);

		dotest(kismet != k0);
		dotest(karma::Karma(k1k) + kismet == k1_n_kismet);
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
