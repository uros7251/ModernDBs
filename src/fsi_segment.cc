#include "moderndbs/segment.h"

using FSISegment = moderndbs::FSISegment;

FSISegment::FSISegment(uint16_t segment_id, BufferManager& buffer_manager, schema::Table& table)
   : Segment(segment_id, buffer_manager), table(table) {
   // TODO: add your implementation herе
}

uint8_t FSISegment::encode_free_space(uint32_t free_space) {
   // TODO: add your implementation here
   auto denominator = buffer_manager.get_page_size() >> 4;
   return static_cast<uint8_t>(free_space/denominator);
}

uint32_t FSISegment::decode_free_space(uint8_t free_space) {
   // TODO: add your implementation here
   auto multiplier = buffer_manager.get_page_size() >> 4;
   return static_cast<uint32_t>(free_space*multiplier);
}

/*uint64_t moderndbs::FSISegment::get_page_count() {
   auto& first_page = buffer_manager.fix_page(static_cast<uint64_t>(segment_id) << 48, false);
   auto page_count = *reinterpret_cast<uint64_t*>(first_page.get_data());
   buffer_manager.unfix_page(first_page, false);
   return page_count;
}*/

void FSISegment::update(uint64_t target_page, uint32_t free_space) {
   // TODO: add your implementation here
   auto last_page_id = (1ull << 47)-1;
   auto& last_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48)^last_page_id, true);
   auto& allocated_pages = *reinterpret_cast<uint64_t*>(last_page.get_data());
   auto page_size = buffer_manager.get_page_size();
   if (target_page == allocated_pages) {
      ++allocated_pages;
      if (target_page%(2*page_size) == 0) {
         buffer_manager.unfix_page(last_page, true);
         auto page_id = target_page/(2*page_size);
         auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
         auto max_free_space = encode_free_space(page_size-sizeof(SlottedPage::Header));
         max_free_space ^= max_free_space << 4;
         auto* byte = reinterpret_cast<uint8_t*>(page.get_data());
         std::memset(byte, max_free_space, page_size);
         *byte = (*byte & 0xF0) ^ (encode_free_space(free_space) & 0x0F);
         buffer_manager.unfix_page(page, true);
         return;
      }
   }
   buffer_manager.unfix_page(last_page, true);
   auto page_id = target_page/(2*page_size);
   auto nibble_id = target_page%(2*page_size);
   auto free_space_encoded = encode_free_space(free_space);
   auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
   auto *byte = reinterpret_cast<uint8_t*>(page.get_data()) + nibble_id/2;
   if (nibble_id % 2) {
      *byte = (*byte & 0x0F) ^ (free_space_encoded << 4);
   }
   else {
      *byte = (*byte & 0xF0) ^ (free_space_encoded & 0x0F);
   }
   buffer_manager.unfix_page(page, true);

   /*auto page_count = get_page_count();

   target_page += 2*sizeof(page_count);
   const auto nibble_per_page = 2*buffer_manager.get_page_size();
   auto page_id = target_page/nibble_per_page;
   auto nibble_id = target_page%nibble_per_page;

   auto free_space_encoded = encode_free_space(free_space);
   auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
   if (page_id == page_count) {
      // update page count
      ++page_count;
      auto& first_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48), true);
      auto* page_count_addr = reinterpret_cast<uint64_t*>(first_page.get_data());
      *page_count_addr = page_count;
      buffer_manager.unfix_page(first_page, true);
      // initialize nibbles
      std::memset(page.get_data(), 0xFF, buffer_manager.get_page_size());
   }
   auto *byte = reinterpret_cast<uint8_t*>(page.get_data()) + nibble_id/2;
   if (nibble_id % 2) {
      *byte = (*byte & 0x0F) ^ (free_space_encoded << 4);
   }
   else {
      *byte = (*byte & 0xF0) ^ (free_space_encoded & 0x0F);
   }
   std::cout.flush();
   buffer_manager.unfix_page(page, true);*/
}

std::optional<uint64_t> FSISegment::find(uint32_t required_space) {
   // TODO: add your implementation here
   auto last_page_id = (1ull << 47)-1;
   auto& last_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48)^last_page_id, false);
   auto allocated_pages = *reinterpret_cast<uint64_t*>(last_page.get_data());
   buffer_manager.unfix_page(last_page, false);
   if (allocated_pages == 0) return {0ull};
   auto page_size = buffer_manager.get_page_size();
   for (auto page_id = 0ull; page_id < allocated_pages/(2*page_size); ++page_id) {
      auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, false);
      auto* byte = reinterpret_cast<uint8_t*>(page.get_data());
      for (auto i=0u; i<page_size;++i) {
         if (required_space <= decode_free_space(byte[i] & 0x0F)) {
            buffer_manager.unfix_page(page, false);
            return {2*(page_id*page_size+i)};
         }
         if (required_space <= decode_free_space(byte[i] >> 4)) {
            buffer_manager.unfix_page(page, false);
            return {2*(page_id*page_size+i)+1};
         }
      }
      buffer_manager.unfix_page(page, false);
   }
   if (allocated_pages%(2*page_size)) {
      auto page_id = allocated_pages/(2*page_size);
      auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, false);
      auto* byte = reinterpret_cast<uint8_t*>(page.get_data());
      auto n = (allocated_pages%(2*page_size)+1)/2; // ceil
      for (auto i=0u; i<n;++i) {
         if (required_space <= decode_free_space(byte[i] & 0x0F)) {
            buffer_manager.unfix_page(page, false);
            return {2*(page_id*page_size+i)};
         }
         if (required_space <= decode_free_space(byte[i] >> 4)) {
            buffer_manager.unfix_page(page, false);
            return {2*(page_id*page_size+i)+1};
         }
      }
      buffer_manager.unfix_page(page, false);
   }
   return {allocated_pages};
   // first page
   /*auto& first_page = buffer_manager.fix_page(static_cast<uint64_t>(segment_id) << 48, false);
   auto page_count = *reinterpret_cast<uint64_t*>(first_page.get_data());
   if (page_count == 0) return {0ull};
   const auto nibble_per_page = 2*buffer_manager.get_page_size();
   auto* byte = reinterpret_cast<uint8_t*>(first_page.get_data()) + sizeof(page_count);
   auto page_size = buffer_manager.get_page_size()-sizeof(page_count);
   for (auto i=0ul; i<page_size; ++i) {
      if (required_space <= decode_free_space(byte[i] & 0x0F)) {
         buffer_manager.unfix_page(first_page, false);
         return {2*i};
      }
      if (required_space <= decode_free_space(byte[i] >> 4)) {
         buffer_manager.unfix_page(first_page, false);
         return {2*i+1};
      }
   }
   buffer_manager.unfix_page(first_page, false);
   // other pages
   for (auto current_page_id = 1ull; current_page_id < page_count; ++current_page_id) {
      auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ current_page_id, false);
      auto* byte = reinterpret_cast<uint8_t*>(page.get_data());
      auto page_size = buffer_manager.get_page_size();
      for (auto i=0ul; i<page_size; ++i) {
         if (required_space <= decode_free_space(byte[i] & 0x0F)) {
            buffer_manager.unfix_page(page, false);
            return {current_page_id*nibble_per_page-2*sizeof(page_count)+2*i};
         }
         if (required_space <= decode_free_space(byte[i] >> 4)) {
            buffer_manager.unfix_page(page, false);
            return {current_page_id*nibble_per_page-2*sizeof(page_count)+2*i+1};
         }
      }
      buffer_manager.unfix_page(page, false);
   }
   // no space found
   if (page_count <= (0x01ull << 47)) return {page_count*nibble_per_page-2*sizeof(page_count)};
   else return {};*/
}
