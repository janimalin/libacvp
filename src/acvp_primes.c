/** @file */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "acvp.h"
#include "acvp_lcl.h"
#include "parson.h"
#include "safe_lib.h"

static ACVP_RESULT acvp_primes_output_tc(ACVP_CTX *ctx,
                                              ACVP_PRIMES_TC *stc,
                                              JSON_Object *tc_rsp) {
    ACVP_RESULT rv = ACVP_SUCCESS;
    char *tmp = NULL;

    tmp = calloc(ACVP_PRIMES_STR_MAX + 1, sizeof(char));
    if (!tmp) {
        ACVP_LOG_ERR("Unable to malloc in acvp_primes_output_mct_tc");
        return ACVP_MALLOC_FAIL;
    }

    if (stc->mode == ACVP_PRIMES_MODE_KEYVER) {
        json_object_set_boolean(tc_rsp, "testPassed", stc->passed);
        goto end;
    }

    memzero_s(tmp, ACVP_PRIMES_STR_MAX);
    rv = acvp_bin_to_hexstr(stc->x, stc->xlen, tmp, ACVP_PRIMES_STR_MAX);
    if (rv != ACVP_SUCCESS) {
        ACVP_LOG_ERR("hex conversion failure (x)");
        goto end;
    }
    json_object_set_string(tc_rsp, "x", tmp);

    memzero_s(tmp, ACVP_PRIMES_STR_MAX);
    rv = acvp_bin_to_hexstr(stc->y, stc->ylen, tmp, ACVP_PRIMES_STR_MAX);
    if (rv != ACVP_SUCCESS) {
        ACVP_LOG_ERR("hex conversion failure (y)");
        goto end;
    }
    json_object_set_string(tc_rsp, "y", tmp);
end:
    if (tmp) free(tmp);

    return rv;
}

static ACVP_RESULT acvp_primes_release_tc(ACVP_PRIMES_TC *stc) {
    if (stc->x) free(stc->x);
    if (stc->y) free(stc->y);
    memzero_s(stc, sizeof(ACVP_PRIMES_TC));
    return ACVP_SUCCESS;
}

static ACVP_PRIMES_TEST_TYPE read_test_type(const char *str) {
    int diff = 1;

    strcmp_s("AFT", 3, str, &diff);
    if (!diff) return ACVP_PRIMES_TT_AFT;

    strcmp_s("VAL", 3, str, &diff);
    if (!diff) return ACVP_PRIMES_TT_VAL;

    return 0;
}

static ACVP_PRIMES_TEST_TYPE read_primegroup(const char *str) {
    int diff = 1;

    strcmp_s("MODP-2048", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_MODP_2048;
    strcmp_s("MODP-3072", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_MODP_3072;
    strcmp_s("MODP-4096", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_MODP_4096;
    strcmp_s("MODP-6144", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_MODP_6144;
    strcmp_s("MODP-8192", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_MODP_8192;
    strcmp_s("ffdhe2048", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_FFDHE_2048;
    strcmp_s("ffdhe3072", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_FFDHE_3072;
    strcmp_s("ffdhe4096", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_FFDHE_4096;
    strcmp_s("ffdhe6144", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_FFDHE_6144;
    strcmp_s("ffdhe8192", 9, str, &diff);
    if (!diff) return ACVP_PRIMES_GROUP_FFDHE_8192;

    return 0;
}

static ACVP_RESULT acvp_primes_init_tc(ACVP_CTX *ctx,
                                       ACVP_PRIMES_TC *stc,
                                       ACVP_PRIMES_MODE mode,
                                       ACVP_PRIMES_GROUPS primegroup,
                                       const char *primegroup_str,
                                       const char *x,
                                       const char *y,
                                       ACVP_PRIMES_TEST_TYPE test_type) {
    ACVP_RESULT rv;
    
    stc->mode = mode;
    stc->test_type = test_type;
    stc->safePrimeGroup = primegroup;
    stc->safePrimeGroupString = primegroup_str;
    
    stc->x = calloc(1, ACVP_PRIMES_BYTE_MAX);
    if (!stc->x) { return ACVP_MALLOC_FAIL; }
    if (x) {
        rv = acvp_hexstr_to_bin(x, stc->x, ACVP_PRIMES_BYTE_MAX, &(stc->xlen));
        if (rv != ACVP_SUCCESS) {
            ACVP_LOG_ERR("Hex conversion failure (x)");
            return rv;
        }
    }
    
    stc->y = calloc(1, ACVP_PRIMES_BYTE_MAX);
    if (!stc->y) { return ACVP_MALLOC_FAIL; }
    if (y) {
        rv = acvp_hexstr_to_bin(y, stc->y, ACVP_PRIMES_BYTE_MAX, &(stc->ylen));
        if (rv != ACVP_SUCCESS) {
            ACVP_LOG_ERR("Hex conversion failure (y)");
            return rv;
        }
    }
    
    return ACVP_SUCCESS;
}
    
static ACVP_RESULT acvp_primes_tc(ACVP_CTX *ctx,
                                  ACVP_CAPS_LIST *cap,
                                  int alg_id,
                                  ACVP_TEST_CASE *tc,
                                  ACVP_PRIMES_TC *stc,
                                  JSON_Object *obj,
                                  JSON_Array *r_garr) {
    JSON_Value *groupval;
    JSON_Object *groupobj = NULL;
    JSON_Array *groups;
    JSON_Value *testval;
    JSON_Object *testobj = NULL;
    JSON_Array *tests, *r_tarr = NULL;
    JSON_Value *r_tval = NULL, *r_gval = NULL;  /* Response testval, groupval */
    JSON_Object *r_tobj = NULL, *r_gobj = NULL; /* Response testobj, groupobj */
    unsigned int i, g_cnt;
    int j, t_cnt, tc_id;
    ACVP_RESULT rv;
    
    const char *test_type_str = NULL, *primegroup_str = NULL;
    ACVP_PRIMES_MODE mode = (alg_id == ACVP_PRIMES_KEYGEN) ? ACVP_PRIMES_MODE_KEYGEN : ACVP_PRIMES_MODE_KEYVER;
    ACVP_PRIMES_GROUPS primegroup = 0;
    ACVP_PRIMES_TEST_TYPE test_type;

    groups = json_object_get_array(obj, "testGroups");
    g_cnt = json_array_get_count(groups);
    
    for (i = 0; i < g_cnt; i++) {
        int tgId = 0;        
        
        groupval = json_array_get_value(groups, i);
        groupobj = json_value_get_object(groupval);

        /*
         * Create a new group in the response with the tgid
         * and an array of tests
         */
        r_gval = json_value_init_object();
        r_gobj = json_value_get_object(r_gval);
        tgId = json_object_get_number(groupobj, "tgId");
        if (!tgId) {
            ACVP_LOG_ERR("Missing tgid from server JSON groub obj");
            rv = ACVP_MALFORMED_JSON;
            goto err;
        }
        json_object_set_number(r_gobj, "tgId", tgId);
        json_object_set_value(r_gobj, "tests", json_value_init_array());
        r_tarr = json_object_get_array(r_gobj, "tests");

        primegroup_str = json_object_get_string(groupobj, "safePrimeGroup");
        if (!primegroup_str) {
            ACVP_LOG_ERR("Missing safePrimeGroup");
            rv = ACVP_INVALID_ARG;
            goto err;
        }

        primegroup = read_primegroup(primegroup_str);
        if (!primegroup) {
            ACVP_LOG_ERR("Server JSON invalid 'safePrimeGroup'");
            rv = ACVP_INVALID_ARG;
            goto err;
        }

        test_type_str = json_object_get_string(groupobj, "testType");
        if (!test_type_str) {
            ACVP_LOG_ERR("Server JSON missing 'testType'");
            rv = ACVP_MISSING_ARG;
            goto err;
        }

        test_type = read_test_type(test_type_str);
        if (!test_type) {
            ACVP_LOG_ERR("Server JSON invalid 'testType'");
            rv = ACVP_INVALID_ARG;
            goto err;
        }

        ACVP_LOG_VERBOSE("    Test group: %d", i);
        ACVP_LOG_VERBOSE("      test type: %s", test_type_str);
        ACVP_LOG_VERBOSE("     primegroup: %s", primegroup_str);

        tests = json_object_get_array(groupobj, "tests");
        t_cnt = json_array_get_count(tests);

        for (j = 0; j < t_cnt; j++) {
            const char *x = NULL, *y = NULL;

            ACVP_LOG_VERBOSE("Found new PRIMES Component test vector...");
            testval = json_array_get_value(tests, j);
            testobj = json_value_get_object(testval);
            tc_id = json_object_get_number(testobj, "tcId");

            if (test_type != ACVP_PRIMES_TT_AFT) {
                ACVP_LOG_ERR("Invalid test type");
                rv = ACVP_INVALID_ARG;
                goto err;
            }

            x = json_object_get_string(testobj, "x");
            y = json_object_get_string(testobj, "y");
            
            /*
             * Create a new test case in the response
             */
            r_tval = json_value_init_object();
            r_tobj = json_value_get_object(r_tval);

            json_object_set_number(r_tobj, "tcId", tc_id);
            /*
             * Setup the test case data that will be passed down to
             * the crypto module.
             */
            rv = acvp_primes_init_tc(ctx, stc, mode, primegroup, primegroup_str, x, y, test_type);
            if (rv != ACVP_SUCCESS) {
                acvp_primes_release_tc(stc);
                json_value_free(r_tval);
                goto err;
            }

            /* Process the current KAT test vector... */
            if ((cap->crypto_handler)(tc)) {
                acvp_primes_release_tc(stc);
                ACVP_LOG_ERR("crypto module failed the operation");
                rv = ACVP_CRYPTO_MODULE_FAIL;
                json_value_free(r_tval);
                goto err;
            }

            /*
             * Output the test case results using JSON
             */
            rv = acvp_primes_output_tc(ctx, stc, r_tobj);
            if (rv != ACVP_SUCCESS) {
                ACVP_LOG_ERR("JSON output failure in PRIMES module");
                acvp_primes_release_tc(stc);
                json_value_free(r_tval);
                goto err;
            }

            /*
             * Release all the memory associated with the test case
             */
            acvp_primes_release_tc(stc);

            /* Append the test response value to array */
            json_array_append_value(r_tarr, r_tval);
        }
        json_array_append_value(r_garr, r_gval);
    }
    rv = ACVP_SUCCESS;

err:
    if (rv != ACVP_SUCCESS) {
        json_value_free(r_gval);
    }
    return rv;
}

static ACVP_RESULT acvp_primes_kat_handler(ACVP_CTX *ctx, JSON_Object *obj, int alg_id) {
    JSON_Value *r_vs_val = NULL;
    JSON_Object *r_vs = NULL;
    JSON_Array *r_garr = NULL; /* Response testarray */
    JSON_Value *reg_arry_val = NULL;
    JSON_Array *reg_arry = NULL;
    JSON_Object *reg_obj = NULL;
    ACVP_CAPS_LIST *cap;
    ACVP_TEST_CASE tc;
    ACVP_PRIMES_TC stc;
    ACVP_RESULT rv = ACVP_SUCCESS;
    const char *alg_str = NULL;
    char *json_result = NULL;

    if (!ctx) {
        ACVP_LOG_ERR("No ctx for handler operation");
        return ACVP_NO_CTX;
    }

    alg_str = json_object_get_string(obj, "algorithm");
    if (!alg_str) {
        ACVP_LOG_ERR("unable to parse 'algorithm' from JSON");
        return ACVP_MALFORMED_JSON;
    }

    /*
     * Get a reference to the abstracted test case
     */
    tc.tc.primes = &stc;
    memzero_s(&stc, sizeof(ACVP_PRIMES_TC));

    /*
     * Create ACVP array for response
     */
    rv = acvp_create_array(&reg_obj, &reg_arry_val, &reg_arry);
    if (rv != ACVP_SUCCESS) {
        ACVP_LOG_ERR("Failed to create JSON response struct. ");
        return rv;
    }

    /*
     * Start to build the JSON response
     */
    rv = acvp_setup_json_rsp_group(&ctx, &reg_arry_val, &r_vs_val, &r_vs, alg_str, &r_garr);
    if (rv != ACVP_SUCCESS) {
        ACVP_LOG_ERR("Failed to setup json response");
        return rv;
    }
    cap = acvp_locate_cap_entry(ctx, alg_id);
    if (!cap) {
        ACVP_LOG_ERR("ACVP server requesting unsupported capability");
        rv = ACVP_UNSUPPORTED_OP;
        goto err;
    }
    rv = acvp_primes_tc(ctx, cap, alg_id, &tc, &stc, obj, r_garr);
    if (rv != ACVP_SUCCESS) {
        goto err;
    }
    json_array_append_value(reg_arry, r_vs_val);

    json_result = json_serialize_to_string_pretty(ctx->kat_resp, NULL);
    ACVP_LOG_VERBOSE("\n\n%s\n\n", json_result);
    json_free_serialized_string(json_result);
    rv = ACVP_SUCCESS;

err:
    if (rv != ACVP_SUCCESS) {
        acvp_primes_release_tc(&stc);
        json_value_free(r_vs_val);
    }
    return rv;
}

ACVP_RESULT acvp_primes_keygen_kat_handler(ACVP_CTX *ctx, JSON_Object *obj) {
    return acvp_primes_kat_handler(ctx, obj, ACVP_PRIMES_KEYGEN);
}

ACVP_RESULT acvp_primes_keyver_kat_handler(ACVP_CTX *ctx, JSON_Object *obj) {
    return acvp_primes_kat_handler(ctx, obj, ACVP_PRIMES_KEYVER);
}
