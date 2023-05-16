#ifndef INCLUDE_MODERNDBS_SEGMENT_H_
#define INCLUDE_MODERNDBS_SEGMENT_H_

#include "moderndbs/buffer_manager.h"
#include "moderndbs/schema.h"
#include "moderndbs/slotted_page.h"
#include <array>
#include <optional>

namespace moderndbs {

class Segment {
   public:
   /// Constructor.
   /// @param[in] segment_id       Id of the segment.
   /// @param[in] buffer_manager   The buffer manager that should be used by the segment.
   Segment(uint16_t segment_id, BufferManager& buffer_manager)
      : segment_id(segment_id), buffer_manager(buffer_manager) {}

   /// The segment id
   uint16_t segment_id;

   protected:
   /// The buffer manager
   BufferManager& buffer_manager;
};

class SchemaSegment : public Segment {
   friend class SPSegment;
   friend class FSISegment;

   public:
   /// Constructor
   /// @param[in] segment_id       Id of the segment that the schema is stored in.
   /// @param[in] buffer_manager   The buffer manager that should be used by the schema segment.
   SchemaSegment(uint16_t segment_id, BufferManager& buffer_manager);
   /// Destructor
   ~SchemaSegment();

   SchemaSegment(const SchemaSegment&) = delete;
   SchemaSegment(SchemaSegment&&) = delete;
   SchemaSegment& operator=(const SchemaSegment&) = delete;
   SchemaSegment& operator=(SchemaSegment&&) = delete;

   /// Set the schema of the schema segment
   void set_schema(std::unique_ptr<schema::Schema> new_schema) { schema = std::move(new_schema); }
   /// Get the schema of the schema segment
   schema::Schema* get_schema() { return schema.get(); }

   /// Read the schema from disk.
   /// The schema segment is structured as follows:
   /// [0-8[   Schema string length in #bytes
   /// [8-?]   schema::Schema object serialized as JSON
   /// Note that the serialized schema *could* be larger than 1 page.
   void read();

   /// Write the schema to disk.
   /// Note that we need to track the number of slotted pages in the schema segment.
   /// For this assignment, you can simply write the schema segment to disk whenever you allocate a slotted page.
   void write();

   protected:
   /// The schema
   std::unique_ptr<schema::Schema> schema;
};

class FSISegment : public Segment {
   public:
   /// Constructor
   /// @param[in] segment_id       Id of the segment that the fsi is stored in.
   /// @param[in] buffer_manager   The buffer manager that should be used by the fsi segment.
   /// @param[in] table            The table that the fsi belongs to.
   FSISegment(uint16_t segment_id, BufferManager& buffer_manager, schema::Table& table);

   /// Update a the free space of a page.
   /// The free space inventory encodes the free space of a target page in 4 bits.
   /// It is left up to you whether you want to implement completely linear free space entries
   /// or half logarithmic ones. (cf. lecture slides)
   ///
   /// @param[in] target_page      The (slotted) page number.
   /// @param[in] free_space       The new free space on that page.
   void update(uint64_t target_page, uint32_t free_space);

   /// Find a free page
   /// @param[in] required_space       The required space.
   std::optional<uint64_t> find(uint32_t required_space);

   /// Encode free space nibble
   uint8_t encode_free_space(uint32_t free_space);
   /// Decode free space nibble
   uint32_t decode_free_space(uint8_t free_space);

   /// The table
   schema::Table& table;
};

class SPSegment : public moderndbs::Segment {
   public:
   /// Constructor
   /// @param[in] segment_id       Id of the segment that the slotted pages are stored in.
   /// @param[in] buffer_manager   The buffer manager that should be used by the slotted pages segment.
   /// @param[in] schema           The schema segment that the fsi belongs to.
   /// @param[in] fsi              The free-space inventory that is associated with the schema.
   /// @param[in] table            The table that the fsi belongs to.
   SPSegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment& schema, FSISegment& fsi, schema::Table& table);

   /// Allocate a new record.
   /// Returns a TID that stores the page as well as the slot of the allocated record.
   /// The allocate method should use the free-space inventory to find a suitable page quickly.
   /// @param[in] size         The size that should be allocated.
   TID allocate(uint32_t size);

   /// Read the data of the record into a buffer.
   /// @param[in] tid          The TID that identifies the record.
   /// @param[in] record       The buffer that is read into.
   /// @param[in] capacity     The capacity of the buffer that is read into.
   /// @return                 The bytes that have been read.
   uint32_t read(TID tid, std::byte* record, uint32_t capacity) const;

   /// Write a record.
   /// @param[in] tid          The TID that identifies the record.
   /// @param[in] record       The buffer that is written.
   /// @param[in] record_size  The capacity of the buffer that is written.
   /// @return                 The bytes that have been written.
   uint32_t write(TID tid, std::byte* record, uint32_t record_size);

   /// Resize a record.
   /// Resize should first check whether the new size still fits on the page.
   /// If not, it yould create a redirect record.
   /// @param[in] tid          The TID that identifies the record.
   /// @param[in] new_length   The new length of the record.
   void resize(TID tid, uint32_t new_length);

   /// Removes the record from the slotted page
   /// @param[in] tid          The TID that identifies the record.
   void erase(TID tid);

   protected:
   /// Schema segment
   SchemaSegment& schema;
   /// Free space inventory
   FSISegment& fsi;
   /// The table
   schema::Table& table;
};

} // namespace moderndbs

#endif // INCLUDE_MODERNDBS_SEGMENT_H_
