#include "moderndbs/slotted_page.h"
#include <cstring>
#include <algorithm>

using moderndbs::SlottedPage;

SlottedPage::Header::Header(uint32_t page_size)
   : slot_count(0),
     first_free_slot(0),
     data_start(page_size),
     free_space(page_size - sizeof(Header)) {}

SlottedPage::SlottedPage(uint32_t page_size)
   : header(page_size) {
   std::memset(get_data() + sizeof(SlottedPage), 0x00, page_size - sizeof(SlottedPage));
}

std::byte *SlottedPage::get_data() {
   return reinterpret_cast<std::byte*>(this);
}

const std::byte *SlottedPage::get_data() const {
   return reinterpret_cast<const std::byte*>(this);
}

SlottedPage::Slot *SlottedPage::get_slots() {
   return reinterpret_cast<SlottedPage::Slot*>(get_data() + sizeof(SlottedPage));
}

const SlottedPage::Slot *SlottedPage::get_slots() const {
   return reinterpret_cast<const SlottedPage::Slot*>(get_data() + sizeof(SlottedPage));
}

uint32_t SlottedPage::get_fragmented_free_space() {
   // TODO: add your implementation here
   return header.data_start - (sizeof(SlottedPage::Header)+sizeof(SlottedPage::Slot)*header.slot_count);
}

uint16_t SlottedPage::allocate(uint32_t data_size, uint32_t page_size) {
   // TODO: add your implementation here
   if (data_size+sizeof(SlottedPage::Slot) > header.free_space) {
      throw std::runtime_error("Not enough space!");
   }
   // if the first free slot is not one of the allocated slots, we require more space
   auto required_space = data_size + (header.first_free_slot == header.slot_count ? sizeof(SlottedPage::Slot) : 0ul);
   if (required_space > get_fragmented_free_space()) {
      compactify(page_size);
   }
   // find and set new slot
   auto slot_id = header.first_free_slot;
   auto* const slots = get_slots();
   auto& slot = slots[slot_id];
   slot.set_slot(header.data_start-data_size, data_size, false);
   // update header
   header.free_space -= required_space;
   header.data_start -= data_size;
   // if first_free_slot wasn't some of the already allocated slots, increase slot_count and first_free_slot
   if (header.first_free_slot == header.slot_count) {
      ++header.slot_count;
      ++header.first_free_slot;
   }
   else {
      // find the next free slot
      for (++header.first_free_slot; header.first_free_slot < header.slot_count && !slots[header.first_free_slot].is_empty(); ++header.first_free_slot);
   }
   return slot_id;
   
}

void SlottedPage::relocate(uint16_t slot_id, uint32_t data_size, uint32_t page_size) {
   // TODO: add your implementation here
   auto* slot = get_slots()+slot_id;
   // trivial case
   if (data_size == slot->get_size()) return;
   // increase free_space (temporarily)
   header.free_space += slot->get_size();
   if (data_size < slot->get_size()) {
      // another trivial case
      slot->set_slot(slot->get_offset(), data_size, slot->is_redirect_target());
      header.free_space -= data_size;
      return;
   }
   else if (data_size > header.free_space) {
      throw std::runtime_error("Not enough space here!");
   }
   auto old_size = slot->get_size();
   auto old_offset = slot->get_offset();
   auto is_redirect_target = slot->is_redirect_target();
   auto* content = new std::byte[old_size];
   std::memcpy(content, get_data()+old_offset, old_size);
   if (data_size > get_fragmented_free_space()) {
      slot->clear();
      compactify(page_size);
   }
   slot->set_slot(header.data_start-data_size, data_size, is_redirect_target);
   std::memcpy(get_data()+slot->get_offset(), content, old_size);
   delete[] content;
   // update header
   header.free_space -= data_size;
   header.data_start -= data_size;
}

void SlottedPage::erase(uint16_t slot_id) {
   // TODO: add your implementation here
   auto* slot = get_slots()+slot_id;
   // record new free space
   if (!slot->is_redirect()) {
      header.free_space+=slot->get_size();
      if (slot->get_offset() == header.data_start) {
         header.data_start+=slot->get_size();
      }
   }
   if (slot_id == header.slot_count-1) {
      // if the slot is the last known valid slot
      auto* slots = get_slots();
      int i = slot_id-1;
      // go back to the next valid slot
      for (; i >= header.first_free_slot && slots[i].is_empty(); --i);
      // update slot_count and first_free_slot
      header.slot_count = static_cast<uint16_t>(i+1);
      header.first_free_slot = std::min(header.first_free_slot, header.slot_count);
      // update free memory 
      header.free_space += sizeof(SlottedPage::Slot)*(slot_id-i);
   }
   else if (slot_id < header.first_free_slot) {
      // declare slot_id to be the new first_free_slot
      header.first_free_slot = slot_id;
   }
   slot->clear();
}

void SlottedPage::make_redirect(uint16_t slot_id, TID redirect_tid) {
   auto* slot = get_slots()+slot_id;
   if (!slot->is_redirect()) {
      header.free_space+=slot->get_size();
      if (slot->get_offset() == header.data_start) {
         header.data_start+=slot->get_size();
      }
   }
   slot->set_redirect_tid(redirect_tid);
}

void SlottedPage::compactify(uint32_t page_size) {
   // TODO: add your implementation here
   auto* data = get_data();
   auto* const slots = get_slots();
   // we first extract on-page slots and arrange them in the order of descending offset
   // sorting will be important for compactifying data
   std::vector<std::pair<uint16_t, uint32_t>> on_page_slots;
   // next line is optional
   on_page_slots.reserve(header.slot_count);
   for (uint16_t i=0; i<header.slot_count; ++i) {
      if (!slots[i].is_empty() && !slots[i].is_redirect()) {
         on_page_slots.emplace_back(i, slots[i].get_offset());
      }
   }
   std::sort(on_page_slots.begin(), on_page_slots.end(), [](const std::pair<uint16_t, uint32_t> s1, const std::pair<uint16_t, uint32_t> s2) {
      return s1.second > s2.second;
   });
   uint32_t write_offset = page_size;
   uint16_t slot_id {0};
   // we then iterate over sorted slots, squishing the data to the end of the page (skipping redirect targets along the way)
   for (auto& pair: on_page_slots) {
      slot_id = pair.first;
      auto size = slots[slot_id].get_size();
      auto read_offset = slots[slot_id].get_offset();
      write_offset-=size;
      std::memmove(data+write_offset, data+read_offset, size);
      // update slot to reflect new offset
      slots[slot_id].set_slot(write_offset, size, slots[slot_id].is_redirect_target());
   }
   // finally, we update data_start to reflect new layout
   header.data_start = write_offset;
}