#include "moderndbs/segment.h"

using FSISegment = moderndbs::FSISegment;

FSISegment::FSISegment(uint16_t segment_id, BufferManager& buffer_manager, schema::Table& table)
   : Segment(segment_id, buffer_manager), table(table) {
   // TODO: add your implementation here
}

uint8_t FSISegment::encode_free_space(uint32_t free_space) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

uint32_t FSISegment::decode_free_space(uint8_t free_space) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

void FSISegment::update(uint64_t target_page, uint32_t free_space) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

std::optional<uint64_t> FSISegment::find(uint32_t required_space) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}
