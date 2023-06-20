// ---------------------------------------------------------------------------------------------------
// MODERNDBS
// ---------------------------------------------------------------------------------------------------
#include "benchmark/benchmark.h"
#include "moderndbs/algebra.h"
#include <vector>
#include <random>
#include <unordered_map>
#include <unordered_set>
// ---------------------------------------------------------------------------------------------------
namespace {
using moderndbs::iterator_model::Register;
using moderndbs::iterator_model::Select;
using moderndbs::iterator_model::Projection;
using moderndbs::iterator_model::Sort;
using moderndbs::iterator_model::HashJoin;
using moderndbs::iterator_model::HashAggregation;

Register convert_to_register(int64_t value) {
   return Register::from_int(value);
}


Register convert_to_register(const std::string& value) {
   return Register::from_string(value);
}


template <typename... Ts, size_t... Is>
void write_to_registers_impl(
   std::vector<Register>& registers,
   const std::tuple<Ts...>& tuple,
   std::index_sequence<Is...>
) {
   ((registers[Is] = convert_to_register(std::get<Is>(tuple))),...);
}


template <typename... Ts>
void write_to_registers(std::vector<Register>& registers, const std::tuple<Ts...>& tuple) {
   write_to_registers_impl(registers, tuple, std::index_sequence_for<Ts...>{});
}


template <typename... Ts>
class TestTupleSource
   : public moderndbs::iterator_model::Operator {
   private:
   const std::vector<std::tuple<Ts...>>& tuples;
   size_t current_index = 0;
   std::vector<Register> output_regs;

   public:
   bool opened = false;
   bool closed = false;

   explicit TestTupleSource(const std::vector<std::tuple<Ts...>>& tuples) : tuples(tuples) {}

   void open() override {
      output_regs.resize(sizeof...(Ts));
      opened = true;
   }

   bool next() override {
      if (current_index < tuples.size()) {
         write_to_registers(output_regs, tuples[current_index]);
         ++current_index;
         return true;
      } else {
         return false;
      }
   }

   void close() override {
      output_regs.clear();
      closed = true;
   }

   std::vector<Register*> get_output() override {
      std::vector<Register*> output;
      output.reserve(sizeof...(Ts));
      for (auto& reg : output_regs) {
         output.push_back(&reg);
      }
      return output;
   }
};

std::string random_string() {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(16);

    for (int i = 0; i < 16; ++i) {
        tmp_s += alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    
    return tmp_s;
}

void Iterator(benchmark::State& state) {
    const int NUM_S = 100000;
    const int NUM_A = 3*NUM_S;
    std::mt19937_64 engine{0};
    std::uniform_int_distribution<int64_t> matrnr{0, NUM_S};
    std::uniform_int_distribution<int64_t> lecnr{0, 200};
    std::vector<std::tuple<int64_t, std::string>> relation_students;
    relation_students.reserve(NUM_S);
    for (size_t i = 0; i < NUM_S; ++i) {
        relation_students.push_back({i, random_string()});
    }
    std::vector<std::tuple<int64_t, int64_t>> relation_attending;
    relation_attending.reserve(NUM_A);
    std::unordered_map<int64_t, std::unordered_set<int64_t>> attending;
    /*
     * SELECT s.name
     * FROM relation_students s, relation_attending a
     * WHERE s.matrnr = a.matrnr and s.matrnr > 1234567
     * GROUP BY s.matrnr, s.name
     * ORDER BY COUNT(a.lecnr) DESC
     */
    for (size_t i = 0; i < NUM_A; ++i) {
        auto student = matrnr(engine);
        auto lecture = lecnr(engine);
        while(attending.contains(student) && attending.at(student).contains(lecture)) {
            lecture = lecnr(engine);
        }
        relation_attending.push_back({student, lecture});
        if(attending.contains(student)) {
            attending.at(student).insert(lecture);    
        } else {
            attending.insert({student, {lecture}});

        }
    }

    for (auto _ : state) {
        TestTupleSource source_students{relation_students};
        TestTupleSource source_attending{relation_attending};
        Select select{source_students, Select::PredicateAttributeInt64{0, NUM_S / 6, Select::PredicateType::GT}};
        HashJoin join{select, source_attending, 0, 0};
        HashAggregation aggregation{
            join,
            {0, 1},
            {HashAggregation::AggrFunc{HashAggregation::AggrFunc::COUNT, 3}}
        };
        Sort sort{aggregation, {{2, true}}};
        Projection projection{sort, {1}};

        projection.open();
        while (projection.next()) {}
        projection.close();
     }
}
} //namespace

BENCHMARK(Iterator)->UseRealTime()->MinTime(10);
