// ---------------------------------------------------------------------------------------------------
// MODERNDBS
// ---------------------------------------------------------------------------------------------------
#include "benchmark/benchmark.h"
#include "moderndbs/slotted_page.h"
#include "moderndbs/buffer_manager.h"
#include "moderndbs/file.h"
#include "moderndbs/hex_dump.h"
#include "moderndbs/segment.h"
#include <algorithm>
#include <vector>
#include <random>
// ---------------------------------------------------------------------------------------------------

using BufferManager = moderndbs::BufferManager;
using FSISegment = moderndbs::FSISegment;
using SPSegment = moderndbs::SPSegment;
using SchemaSegment = moderndbs::SchemaSegment;
using SlottedPage = moderndbs::SlottedPage;
using TID = moderndbs::TID;

namespace schema = moderndbs::schema;

namespace {

std::unique_ptr<schema::Schema> getTPCHSchemaLight() {
   std::vector<schema::Table> tables {
      schema::Table(
         "customer",
         {
            schema::Column("c_custkey", schema::Type::Integer()),
            schema::Column("c_name", schema::Type::Char(25)),
            schema::Column("c_address", schema::Type::Char(40)),
            schema::Column("c_nationkey", schema::Type::Integer()),
            schema::Column("c_phone", schema::Type::Char(15)),
            schema::Column("c_acctbal", schema::Type::Integer()),
            schema::Column("c_mktsegment", schema::Type::Char(10)),
            schema::Column("c_comment", schema::Type::Char(117)),
         },
         {
            "c_custkey"
         },
         10, 11,
         0
         ),
      schema::Table(
         "nation",
         {
            schema::Column("n_nationkey", schema::Type::Integer()),
            schema::Column("n_name", schema::Type::Char(25)),
            schema::Column("n_regionkey", schema::Type::Integer()),
            schema::Column("n_comment", schema::Type::Char(152)),
         },
         {
            "n_nationkey"
         },
         20, 21,
         0
         ),
      schema::Table(
         "region",
         {
            schema::Column("r_regionkey", schema::Type::Integer()),
            schema::Column("r_name", schema::Type::Char(25)),
            schema::Column("r_comment", schema::Type::Char(152)),
         },
         {
            "r_regionkey"
         },
         30, 31,
         0
         ),
   };
   auto schema = std::make_unique<schema::Schema>(std::move(tables));
   return schema;
}

void SlottedPages(benchmark::State& state) {
   BufferManager buffer_manager(4096, 100);
   SchemaSegment schema_segment(0, buffer_manager);
   schema_segment.set_schema(getTPCHSchemaLight());
   auto& table = schema_segment.get_schema()->tables[0];

   constexpr auto record_size = sizeof(uint64_t);
   constexpr auto max = 4096 - sizeof(SlottedPage::Header);
   constexpr auto max_records = max / (record_size + sizeof(SlottedPage::Slot) + sizeof(TID));
   constexpr auto workload_size = 10 * max_records;

   std::mt19937_64 engine{0};
   std::uniform_int_distribution<uint64_t> size{-record_size + 1, record_size};
   std::uniform_int_distribution<uint64_t> factor{2, 16};

   for (auto _ : state) {
      FSISegment fsi_segment(table.fsi_segment, buffer_manager, table);
      SPSegment sp_segment(table.sp_segment, buffer_manager, schema_segment, fsi_segment, table);
      std::vector<TID> tids;
      std::vector<std::byte> writeBuffer;
      writeBuffer.resize(record_size);

      // fill two pages
      for (uint64_t i = 0; i < workload_size * 2; ++i) {
         std::memset(writeBuffer.data(), i, record_size);
         auto tid = sp_segment.allocate(record_size);
         sp_segment.write(tid, writeBuffer.data(), record_size);
         tids.push_back(tid);
      }

      // erase some random records
      std::shuffle(tids.begin(), tids.end(), std::default_random_engine(0));
      for (size_t i = 0; i < workload_size; i++){
         sp_segment.erase(tids[i]);
      }

      tids.erase(tids.begin(), tids.begin() + workload_size);

      // allocate some bigger records
      for (size_t i = 0; i < workload_size / 2; ++i) {
         tids.push_back(sp_segment.allocate(record_size*factor(engine)));
      }


      // resize some random records
      std::shuffle(tids.begin(), tids.end(), std::default_random_engine(0));
      for (size_t i = 0; i < workload_size; i++){
         sp_segment.resize(tids[i], record_size + size(engine));
      }
   }
}
} // namespace

BENCHMARK(SlottedPages)->UseRealTime()->MinTime(10);
