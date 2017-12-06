/*
 * Copyright (c) 2017 Snowflake Computing, Inc. All rights reserved.
 */


#include <stdio.h>
#include <stdlib.h>
#include <snowflake_client.h>
#include <example_setup.h>


int main() {
    /* init */
    SNOWFLAKE_STATUS status;
    initialize_snowflake_example(SF_BOOLEAN_FALSE);
    SNOWFLAKE *sf = NULL;

    // Try connecting with a NULL connection struct, should fail
    status = snowflake_connect(sf);
    if (status != SF_STATUS_SUCCESS) {
        printf("Connecting to snowflake failed\n");
    } else {
        fprintf(stderr, "Connecting to snowflake succeeded...exiting");
        goto cleanup;
    }

    // Try to connect with empty connection struct, should fail
    sf = snowflake_init();
    status = snowflake_connect(sf);
    if (status != SF_STATUS_SUCCESS) {
        printf("Connecting to snowflake failed\n");
        SNOWFLAKE_ERROR *error = snowflake_error(sf);
        printf("Error message: %s\nIn File, %s, Line, %d\n", error->msg, error->file, error->line);
    } else {
        fprintf(stderr, "Connecting to snowflake succeeded...exiting");
        goto cleanup;
    }

    /* query, try running a query with a connection struct that has not connected */
    SNOWFLAKE_STMT *sfstmt = snowflake_stmt(sf);
    SNOWFLAKE_BIND_OUTPUT c1;
    int out = 0;
    c1.idx = 1;
    c1.type = SF_C_TYPE_INT64;
    c1.value = (void *) &out;
    snowflake_bind_result(sfstmt, &c1);
    snowflake_prepare(sfstmt, "select 1;");
    status = snowflake_execute(sfstmt);
    if (status != SF_STATUS_SUCCESS) {
        printf("Running query to snowflake failed\n");
        SNOWFLAKE_ERROR *error = snowflake_stmt_error(sfstmt);
        printf("Error message: %s\nIn File, %s, Line, %d\n", error->msg, error->file, error->line);
    } else {
        fprintf(stderr, "Running query succeeded...exiting");
        goto cleanup;
    }

    // Connect with minimum parameters and retry running query
    snowflake_set_attr(sf, SF_CON_ACCOUNT, getenv("SNOWFLAKE_TEST_ACCOUNT"));
    snowflake_set_attr(sf, SF_CON_USER, getenv("SNOWFLAKE_TEST_USER"));
    snowflake_set_attr(sf, SF_CON_PASSWORD, getenv("SNOWFLAKE_TEST_PASSWORD"));
    status = snowflake_connect(sf);
    if (status != SF_STATUS_SUCCESS) {
        SNOWFLAKE_ERROR *error = snowflake_error(sf);
        fprintf(stderr, "Error message: %s\nIn File, %s, Line, %d\n", error->msg, error->file, error->line);
        printf("Failed connecting with minimum parameters set.\n");
    } else {
        printf("OK, connected with just account, user, password\n");
    }

    // Retry query now that connection works
    status = snowflake_execute(sfstmt);
    if (status != SF_STATUS_SUCCESS) {
        SNOWFLAKE_ERROR *error = snowflake_stmt_error(sfstmt);
        fprintf(stderr, "Error message: %s\nIn File, %s, Line, %d\n", error->msg, error->file, error->line);
        printf("Failed running query with connect snowflake object.\n");
    } else {
        printf("OK, query executed successfully\n");
    }
    printf("Number of rows: %d\n", (int) snowflake_num_rows(sfstmt));

    while ((status = snowflake_fetch(sfstmt)) != SF_STATUS_EOL) {
        if (status == SF_STATUS_ERROR || status == SF_STATUS_WARNING) {
            SNOWFLAKE_ERROR *error = snowflake_stmt_error(sfstmt);
            fprintf(stderr, "Error message: %s\nIn File, %s, Line, %d\n", error->msg, error->file, error->line);
            break;
        }
        printf("result: %d\n", *((int *) c1.value));
    }
    // If we reached end of line, then we were successful
    if (status == SF_STATUS_EOL) {
        status = SF_STATUS_SUCCESS;
    }

cleanup:
    /* Clean up stmt struct*/
    snowflake_stmt_close(sfstmt);
    /* disconnect */
    snowflake_close(sf);

    /* term */
    snowflake_term(sf); // purge snowflake context
    snowflake_global_term();

    return status;
}
