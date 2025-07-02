/**
 * @file app_diagnostics.h
 * @brief Application self-tests and diagnostics.
 *
 * Contains functions to test modules of the application to ensure
 * they are working correctly.
 */

#ifndef APP_DIAGNOSTICS_H
#define APP_DIAGNOSTICS_H

/**
 * @brief Simple write/read test on the storage manager.
 *
 * Verifies that the storage module can write and read from
 * the SPIFFS filesystem.
 */
void diagnostics_run_storage_test(void);

/**
 * @brief Simple spiffs database test.
 *
 * Verifies that the database existis and potentially contains data.
 */
void diagnostics_run_database_test(void);

#endif // APP_DIAGNOSTICS_H