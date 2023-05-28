#include "moderndbs/segment.h"
#include "moderndbs/slotted_page.h"

using moderndbs::SPSegment;
using moderndbs::Segment;
using moderndbs::TID;

SPSegment::SPSegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment &schema, FSISegment &fsi, schema::Table& table)
   : Segment(segment_id, buffer_manager), schema(schema), fsi(fsi), table(table) {
   // TODO: add your implementation here
}

TID SPSegment::allocate(uint32_t size, bool is_redirect_target) {
   // TODO: add your implementation here
   auto result = fsi.find(size);
   if (!result.has_value()) throw std::runtime_error("Not enough memory!");
   auto page_id = *result;
   auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
   auto* slotted_page = reinterpret_cast<SlottedPage*>(page.get_data());
   if (slotted_page->header.data_start == 0) {
      // initialize in place
      slotted_page = new (page.get_data()) SlottedPage(buffer_manager.get_page_size());
   }
   auto slot_id = slotted_page->allocate(size, buffer_manager.get_page_size());
   auto* slot = slotted_page->get_slots()+slot_id;
   if (is_redirect_target) slot->mark_as_redirect_target();
   auto free_space = slotted_page->header.free_space;
   buffer_manager.unfix_page(page, true);
   fsi.update(page_id, free_space);
   return TID(page_id, slot_id);
}

uint32_t SPSegment::read(TID tid, std::byte *record, uint32_t capacity) const {
   // TODO: add your implementation here
   auto& page = buffer_manager.fix_page(tid.get_page_id(segment_id), false);
   auto* sp = reinterpret_cast<SlottedPage*>(page.get_data());
   auto* slot = sp->get_slots() + tid.get_slot();
   auto bytes_read = slot->get_size();
   if (!slot->is_redirect()) {
      auto* src = reinterpret_cast<std::byte*>(page.get_data()) + slot->get_offset();
      if (slot->is_redirect_target()) {
         src+=sizeof(TID);
         bytes_read-=sizeof(TID);
      }
      assert(bytes_read>=capacity);
      bytes_read = std::min(bytes_read, capacity);
      std::memcpy(record, src, bytes_read);
   }
   else {
      bytes_read = read(slot->as_redirect_tid(), record, capacity);
   }
   buffer_manager.unfix_page(page, false);
   return bytes_read;
}

uint32_t SPSegment::write(TID tid, std::byte *record, uint32_t record_size) {
   // TODO: add your implementation here
   auto& page = buffer_manager.fix_page(tid.get_page_id(segment_id), true);
   auto* sp = reinterpret_cast<SlottedPage*>(page.get_data());
   auto* slot = sp->get_slots() + tid.get_slot();
   assert(!slot->is_redirect());
   assert(slot->get_size()>=record_size);
   auto* dst = reinterpret_cast<std::byte*>(page.get_data()) + slot->get_offset();
   std::memcpy(dst, record, record_size);
   buffer_manager.unfix_page(page, true);
   return record_size;
}

void SPSegment::resize(TID tid, uint32_t new_length) {
   // TODO: add your implementation here
   auto& page = buffer_manager.fix_page(tid.get_page_id(segment_id), true);
   auto* sp = reinterpret_cast<SlottedPage*>(page.get_data());
   auto* slot = sp->get_slots() + tid.get_slot();
   if (slot->is_redirect()) {
      auto redirect_tid = slot->as_redirect_tid();
      auto& redirect_page = buffer_manager.fix_page(redirect_tid.get_page_id(segment_id), true);
      auto* redirect_sp = reinterpret_cast<SlottedPage*>(redirect_page.get_data());
      // std::cout << redirect_tid.get_slot() <<"\n";
      auto* redirect_slot = redirect_sp->get_slots() + redirect_tid.get_slot();
      auto old_size = redirect_slot->get_size();
      if (new_length+sizeof(TID) <= redirect_sp->header.free_space+old_size) {
         redirect_sp->relocate(redirect_tid.get_slot(), new_length+sizeof(TID), buffer_manager.get_page_size());
         auto page_id = redirect_tid.get_page_id(0);
         fsi.update(page_id, redirect_sp->header.free_space);
      }
      else {
         auto rredirect_tid = allocate(new_length+sizeof(TID), true);
         write(rredirect_tid, reinterpret_cast<std::byte*>(redirect_sp->get_data()+redirect_slot->get_offset()), redirect_slot->get_size());
         sp->make_redirect(tid.get_slot(), rredirect_tid);
         redirect_sp->erase(redirect_tid.get_slot());
         auto page_id = redirect_tid.get_page_id(0);
         fsi.update(page_id, redirect_sp->header.free_space);
      }
      buffer_manager.unfix_page(redirect_page, true);
   }
   else {
      if (new_length <= sp->header.free_space+slot->get_size()) {
         sp->relocate(tid.get_slot(), new_length, buffer_manager.get_page_size());
         auto page_id = tid.get_page_id(0);
         fsi.update(page_id, sp->header.free_space);
      }
      else {
         auto redirect_tid = allocate(new_length+sizeof(TID), true);
         auto* to_be_written = new std::byte[slot->get_size()+sizeof(TID)];
         std::memcpy(to_be_written, &tid, sizeof(TID));
         std::memcpy(to_be_written+sizeof(TID), sp->get_data()+slot->get_offset(), slot->get_size());
         write(redirect_tid, to_be_written, slot->get_size()+sizeof(TID));
         delete[] to_be_written;
         sp->make_redirect(tid.get_slot(), redirect_tid);
         fsi.update(tid.get_page_id(0), sp->header.free_space);
      }
   }
   buffer_manager.unfix_page(page, true);
}

void SPSegment::erase(TID tid) {
   // TODO: add your implementation here
   auto& page = buffer_manager.fix_page(tid.get_page_id(segment_id), true);
   auto* sp = reinterpret_cast<SlottedPage*>(page.get_data());
   auto* slot = sp->get_slots() + tid.get_slot();
   if (slot->is_redirect()) {
      auto redirect_tid = slot->as_redirect_tid();
      auto& redirect_page = buffer_manager.fix_page(redirect_tid.get_page_id(segment_id), true);
      auto* redirect_sp = reinterpret_cast<SlottedPage*>(redirect_page.get_data());
      if (!redirect_sp->get_slots()[redirect_tid.get_slot()].is_redirect_target()) {
         throw std::runtime_error("Not redirect target");
      }
      redirect_sp->erase(redirect_tid.get_slot());
      buffer_manager.unfix_page(redirect_page, true);
   }
   sp->erase(tid.get_slot());
   buffer_manager.unfix_page(page, true);
}
