#include <nt.h>
#include "cache.h"
#include <limits>

using namespace std;

static const char usageText[] =
"This application is a forwarding daemon that performs deduplication\n"
"on network traffic that is received by the application. This\n"
"deduplication utilizes Napatech DYN4 packet descriptors and correlation\n"
"keys.\n"
"\n"
"Syntax:\n"
"\n"
"   dedupe <rx_stream> <tx_port> <lru_size>\n"
"\n"
"       rx_stream       : the stream to capture on.\n"
"       tx_port         : the port to send traffic from.\n"
"       lru_size        : the size of the packet cache.\n"
"\n";

struct options_t {
    uint32_t stream_id;
    size_t lru_size;
    uint32_t tx_port;
};

options_t parse_args(int argc, char * argv[]);
void help();

int main(int argc, char * argv[]);

class Deduplicator {
    public:
        /*
        * Constructor
        * stream: the identifier for the stream to listen to
        * size: the size of the lru to use to deduplicate traffic
        * port: the transmit port to send from
        */
        Deduplicator(const uint32_t stream, size_t size, uint32_t port);
        /*
        * Destructor
        */
        virtual ~Deduplicator();
        /*
        * Listen serves the deduplication functionality
        */
        virtual void Listen();
    protected:
        virtual void Open();
        virtual void Close();
        virtual void PacketDone();
        virtual void PrintError();
        virtual void Error();
        virtual bool IncrementOK(uint64_t counter);
    
    private:
        uint32_t status; // status code returned from NTAPI calls
        char err[NT_ERRBUF_SIZE]; // error message for last napatech action
        uint32_t stream_id; // the stream to listen on for packets
        size_t lru_size; // the size of the lru to use for deduplication
        uint32_t tx_port; // the port to transmit from

        uint64_t cmax = std::numeric_limits<uint64_t>::max();
        uint64_t seen; // the number of packets seen
        uint64_t discard; // the number of packets discarded
        
        NtNetStreamRx_t rx_stream; // napatech stream to receive from
        NtNetBuf_t rx_buffer; // buffer to hold packet payload
        NtDyn4Descr_t* desc; // pointer to packet descriptor

        NtNetStreamTx_t tx_stream; // napatech stream to send to
        NtNetTxFragment_t packetFragmentlist[1]; // tx packet metadata array
};