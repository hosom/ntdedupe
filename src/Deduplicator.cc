#include <nt.h>
#include "Deduplicator.h"
#include "cache.h"
#include <iostream>
#include <sstream>

Deduplicator::~Deduplicator() {
    Close();
}

Deduplicator::Deduplicator(uint32_t stream, size_t size, uint32_t port) {
    stream_id = stream;
    lru_size = size;
    tx_port = port;

    seen = 0;
    discard = 0;

    Open();
}

void Deduplicator::Open() {
    if ((status = NT_Init(NTAPI_VERSION)) != NT_SUCCESS) {
        Error();
    }

    if ((status = NT_NetRxOpen(&rx_stream, "NtDedupeRx", 
        NT_NET_INTERFACE_PACKET, stream_id, -1)) != NT_SUCCESS ){
        Error();
    }

    if ((status = NT_NetTxOpen(&tx_stream, "NtDedupeTx", 
        (1<<tx_port), NT_NETTX_NUMA_ADAPTER_HB, 0)) != NT_SUCCESS) {
        Error();
    }
}

void Deduplicator::Listen() {
    cache::lru<uint64_t, uint64_t> dedupe_lru = cache::lru<uint64_t, uint64_t>(lru_size);
    while (true) {
        // Obtain the next packet 
        if ((status = NT_NetRxGet(rx_stream, &rx_buffer, 1000)) != NT_SUCCESS) {
            if ((status == NT_STATUS_TIMEOUT) || (status == NT_STATUS_TRYAGAIN)) {
                // Timeout errors are fine. Just wait a little longer...
                continue;
            }
            // An error has occurred
            Error();
        }

        // Need to find a safer way to deal with this counter to prevent
        // integer overflows
        if (IncrementOK(seen)) {
            ++seen;
        } else {
            seen = 1;
            discard = 0;
        }

        desc = NT_NET_DESCR_PTR_DYN4(rx_buffer);
        if (dedupe_lru.exists(desc->color1)) {
            // Again, need to find a safer way to deal with this counter
            if (IncrementOK(discard)) {
                ++discard;
            } else {
                seen = 1;
                discard = 1; 
            }
            PacketDone();
            continue;
        }

        dedupe_lru.add(desc->color1, desc->color1);
        // prepare the packet for transmit
        packetFragmentlist[0].size = NT_NET_GET_PKT_WIRE_LENGTH(rx_buffer);
        packetFragmentlist[0].data = (uint8_t*) NT_NET_GET_PKT_L2_PTR(rx_buffer);

        if ((status = NT_NetTxAddPacket(tx_stream, tx_port, 
            packetFragmentlist, 1, -1)) != NT_SUCCESS) {
            Error();
        }

        PacketDone();
        if (seen % 1000000 == 0) {
            std::cout << "seen=" << seen << " discard=" << discard \
                << " percent=" << (((double)discard/(double)seen) * 100)<< "\n";
        }
    }
}

bool Deduplicator::IncrementOK(uint64_t counter) {
    return !(counter == cmax);
}

void Deduplicator::PacketDone() {
    if ((status = NT_NetRxRelease(rx_stream, rx_buffer)) != NT_SUCCESS) {
        Error();
    }
}

void Deduplicator::Close() {
    cout << "Shutting down...\nClosing receive stream\n";
    if ((status = NT_NetRxClose(rx_stream)) != NT_SUCCESS) {
        PrintError();
    }

    cout << "Closing transmit stream\n";
    if ((status = NT_NetTxClose(tx_stream)) != NT_SUCCESS) {
        PrintError();
    }

    cout << "Closing NTAPI\n";
    if ((status = NT_Done()) != NT_SUCCESS) {
        PrintError();
    }
}

void Deduplicator::PrintError() {
    NT_ExplainError(status, err, NT_ERRBUF_SIZE);
    cout << err;
}

void Deduplicator::Error() {
    PrintError();
    Close();
}

options_t parse_args(int argc, char * argv[]) {
    options_t opts;

    if (argc <= 3)
        help();
    
    std::stringstream c_stream_id(argv[1]);
    if (!(c_stream_id >> opts.stream_id)) {
        std::cout << "Failed to parse rx stream value.\n";
        help();
    }

    std::stringstream c_tx_port(argv[2]);
    if (!(c_tx_port >> opts.tx_port)) {
        std::cout << "Failed to parse tx port value.\n";
        help();
    }

    std::stringstream c_lru_size(argv[3]);
    if (!(c_lru_size >> opts.lru_size)) {
        std::cout << "Failed to parse lru size value.\n";
        help();
    }

    return opts;
}

void help() {
    std::cout << usageText;
    exit(1);
}

int main(int argc, char * argv[]) {
    options_t opts = parse_args(argc, argv);
    Deduplicator app = Deduplicator(opts.stream_id, opts.lru_size, opts.tx_port);

    app.Listen();
}