#include "moderndbs/segment.h"

using FSISegment = moderndbs::FSISegment;

FSISegment::FSISegment(uint16_t segment_id, BufferManager& buffer_manager, schema::Table& table)
   : Segment(segment_id, buffer_manager), table(table) {
   // TODO: add your implementation herе
   // set up encoding_scheme_map 
   unsigned j=1, k=15;
   for (; j<=16 && (1<<j)*(k+1) < buffer_manager.get_page_size()+2; ++j,--k);
   if (j>16) throw std::runtime_error("Page size too big for this encoding scheme!");
   encoding_scheme_map[0] = 0;
   for (unsigned i=1; i<j; ++i) {
      encoding_scheme_map[i]=(1<<i)-1;
   }
   for (unsigned i=j; i<16; ++i) {
      encoding_scheme_map[i] = (i-j+1)*(1<<j)-1;
   }
   auto last_page_id = (1ull << 47)-1;
   auto& last_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48)^last_page_id, true);
   auto* free_space_cache = reinterpret_cast<std::optional<uint64_t>*>(last_page.get_data()+sizeof(uint64_t));
   *free_space_cache = {0};
   for (j=1; j<16; ++j) {
      free_space_cache[j] = find(
         std::min(encoding_scheme_map[j], buffer_manager.get_page_size()),
         false);
   }
   buffer_manager.unfix_page(last_page, true);
}

uint8_t FSISegment::encode_free_space(uint32_t free_space) {
   // TODO: add your implementation here
   // binary search
   uint8_t index=0;
   for (uint8_t n=8; n>0; n/=2) {
      index += (free_space >= encoding_scheme_map[index+n]) ? n : 0;
   }
   return index;
   /*auto denominator = buffer_manager.get_page_size() >> 4;
   return static_cast<uint8_t>(free_space/denominator);*/
}

// uint32_t FSISegment::decode_free_space(uint8_t free_space) {
//    // TODO: add your implementation here
//    auto multiplier = buffer_manager.get_page_size() >> 4;
//    return static_cast<uint32_t>(free_space*multiplier);
// }

void FSISegment::update(uint64_t target_page, uint32_t free_space) {
   // TODO: add your implementation here
   auto last_page_id = (1ull << 47)-1;
   auto& last_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48)^last_page_id, true);
   auto& allocated_pages = *reinterpret_cast<uint64_t*>(last_page.get_data());
   auto page_size = buffer_manager.get_page_size();
   if (target_page == allocated_pages) {
      ++allocated_pages;
      if (target_page%(2*page_size) == 0) {
         auto page_id = target_page/(2*page_size);
         auto& page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
         auto max_free_space = encode_free_space(page_size-sizeof(SlottedPage::Header));
         max_free_space ^= max_free_space << 4;
         auto* byte = reinterpret_cast<uint8_t*>(page.get_data());
         std::memset(byte, max_free_space, page_size);
         *byte = (*byte & 0xF0) ^ (encode_free_space(free_space) & 0x0F);
         buffer_manager.unfix_page(page, true);
         goto end;
      }
   }
   {
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
   }
   end:
   auto* free_space_cache = reinterpret_cast<std::optional<uint64_t>*>(last_page.get_data()+sizeof(uint64_t));
   for (unsigned i=1; i<16; ++i) {
      if (free_space_cache[i].has_value()) {
         auto page_id = *free_space_cache[i];
         if (page_id == target_page && encode_free_space(free_space) < i) {
            free_space_cache[i] = find(encoding_scheme_map[i], false);
         }
      }
   }
   buffer_manager.unfix_page(last_page, true);
}

std::optional<uint64_t> FSISegment::find(uint32_t required_space, bool use_cache) {
   // TODO: add your implementation here
   auto last_page_id = (1ull << 47)-1;
   auto& last_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48)^last_page_id, false);
   auto allocated_pages = *reinterpret_cast<uint64_t*>(last_page.get_data());
   auto* free_space_cache = reinterpret_cast<std::optional<uint64_t>*>(last_page.get_data()+sizeof(uint64_t));
   if (use_cache) {
      auto encoded = encode_free_space(required_space);
      if (encoded < 15 && decode_free_space(encoded) < required_space) ++encoded;
      auto result = free_space_cache[encoded];
      buffer_manager.unfix_page(last_page, false);
      return result;
   }
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
}
