

#include <http_in_c/reader.h>
#include <http_in_c/common/alloc.h>
#include <http_in_c/common/utils.h>
#include <http_in_c/common/iobuffer.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

SyncReaderRef SyncReader_new(ParserRef parser, RdSocket rdsock)
{
    SyncReaderRef rdr = eg_alloc(sizeof(Reader));
    if(rdr == NULL)
        return NULL;
    SyncReader_init(rdr, parser, rdsock);
    return rdr;
}
void SyncReader_init(SyncReaderRef  this, ParserRef parser, RdSocket rdsock)
{
    ASSERT_NOT_NULL(this);
    this->m_parser = parser;
//    this->m_socket = socket;
    this->m_rdsocket = rdsock;
    this->m_iobuffer = IOBuffer_new();
}

void SyncReader_destroy(SyncReaderRef this)
{
    IOBuffer_dispose(&(this->m_iobuffer));

}
void SyncReader_dispose(SyncReaderRef* this_ptr)
{
    SyncReaderRef this = *this_ptr;
    SyncReader_destroy(this);
    eg_free((void*)this);
    *this_ptr = NULL;
}
int SyncReader_read(SyncReaderRef this, MessageRef* msgref_ptr)
{
    IOBufferRef iobuf = this->m_iobuffer;
    MessageRef message_ptr = Message_new();
    Parser_begin(this->m_parser, message_ptr);
    int bytes_read;
    for(;;) {
        // 
        // handle nothing left in iobuffer
        // only read more if iobuffer is empty
        // 
        if(IOBuffer_data_len(iobuf) == 0 ) {
            IOBuffer_reset(iobuf);
            void* buf = IOBuffer_space(iobuf); 
            int len = IOBuffer_space_len(iobuf);
            void* c = this->m_rdsocket.ctx;
            ReadFunc rf = (this->m_rdsocket.read_f);
            bytes_read = RdSocket_read(&(this->m_rdsocket), buf, len);
            if(bytes_read == 0) {
                if (! this->m_parser->m_started) {
                    // eof no message started - there will not be any more bytes to parse so cleanup and exit
                    // return no error 
                    Message_dispose(&(message_ptr));
                    *msgref_ptr = NULL;
                    return 0;
                }
                if (this->m_parser->m_started && this->m_parser->m_message_done) {
                    // shld not get here
                    assert(false);
                }
                if (this->m_parser->m_started && !this->m_parser->m_message_done) {
                    // get here if otherend is signlaling eom with eof
                    assert(true);
                }
            } else if (bytes_read > 0) {
                IOBuffer_commit(iobuf, bytes_read);
            } else {
                // have an io error
                int x = errno;
                Message_dispose(&(message_ptr));
                *msgref_ptr = NULL;
                return READER_IO_ERROR;
            }
        } else {
            bytes_read = iobuf->buffer_remaining;
        }
        char* tmp = (char*)iobuf->buffer_ptr;
        char* tmp2 = (char*)iobuf->mem_p;
        ParserReturnValue ret = Parser_consume(this->m_parser, (void*) IOBuffer_data(iobuf), IOBuffer_data_len(iobuf));
        int consumed = bytes_read - ret.bytes_remaining;
        IOBuffer_consume(iobuf, consumed);
        int tmp_remaining = iobuf->buffer_remaining;
        switch(ret.return_code) {
            case ParserRC_error:
                ///
                /// got a parse error - need some way to signal the caller so can send reply of bad message
                ///
                Message_dispose(&message_ptr);
                *msgref_ptr = NULL;
                return READER_PARSE_ERROR;
                break;
            case ParserRC_end_of_data:
                break;
            case ParserRC_end_of_header:
                break;
            case ParserRC_end_of_message:
                *msgref_ptr = message_ptr;
                // return ok
                return READER_OK;
        }
    }
}
