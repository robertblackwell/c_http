#ifndef c_http_test_helpers_reader_private_h
#define c_http_test_helpers_reader_private_h
/**
 * @addtogroup group_test_helpers SyncReader_private
 * @brief A private interface to Reader module strictly for testing internals of the reader
 * @{
 */

#include <c_http/common/http_parser/rdsocket.h>
#include <c_http/common/message.h>

SyncReaderRef SyncReader_private_new(RdSocket rdsock);
void SyncReader_private__init(SyncReaderRef this, RdSocket rdsock);
/** @} */
#endif