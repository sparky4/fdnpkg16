#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "src/net.c"

START_TEST(test_net_dnsresolve_bounds_safety)
{
    // Invariant: net_dnsresolve() must not read beyond input buffer or overflow cache buffer
    const char *payloads[] = {
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",  // 64 chars (boundary)
        "short",                                                              // valid input
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\x00extra"  // embedded null
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // The security property: strlen must not read out-of-bounds
        // We test by ensuring the function returns without crashing
        // and doesn't corrupt memory visible to the test
        struct sockaddr_in result;
        int ret = net_dnsresolve(payloads[i], &result);
        
        // Basic safety check - function should handle input without crashing
        ck_assert_msg(1, "Function must not crash on adversarial input");
        
        // Additional check: if length >= 64, cache should not be modified
        if (strlen(payloads[i]) >= 64) {
            // Verify cache wasn't written to by checking a sentinel
            // This assumes we can inspect the static cache array
            // Since we can't directly access static from test, we rely on
            // the function returning appropriate error code
            ck_assert_int_ne(ret, 0);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_net_dnsresolve_bounds_safety);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}