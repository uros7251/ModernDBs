#include "moderndbs/segment.h"
#include "moderndbs/slotted_page.h"

using moderndbs::SPSegment;
using moderndbs::Segment;
using moderndbs::TID;

SPSegment::SPSegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment &schema, FSISegment &fsi, schema::Table& table)
   : Segment(segment_id, buffer_manager), schema(schema), fsi(fsi), table(table) {
   // TODO: add your implementation here
}

TID SPSegment::allocate(uint32_t size) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

uint32_t SPSegment::read(TID tid, std::byte *record, uint32_t capacity) const {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

uint32_t SPSegment::write(TID tid, std::byte *record, uint32_t record_size) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

void SPSegment::resize(TID tid, uint32_t new_length) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

void SPSegment::erase(TID tid) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}
