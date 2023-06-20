#include "moderndbs/algebra.h"
#include <cassert>
#include <functional>
#include <string>


namespace moderndbs::iterator_model {


/// This can be used to store registers in an `std::unordered_map` or
/// `std::unordered_set`. Examples:
///
/// std::unordered_map<Register, int, RegisterHasher> map_from_reg_to_int;
/// std::unordered_set<Register, RegisterHasher> set_of_regs;
struct RegisterHasher {
    uint64_t operator()(const Register& r) const {
        return r.get_hash();
    }
};


/// This can be used to store vectors of registers (which is how tuples are
/// represented) in an `std::unordered_map` or `std::unordered_set`. Examples:
///
/// std::unordered_map<std::vector<Register>, int, RegisterVectorHasher> map_from_tuple_to_int;
/// std::unordered_set<std::vector<Register>, RegisterVectorHasher> set_of_tuples;
struct RegisterVectorHasher {
    uint64_t operator()(const std::vector<Register>& registers) const {
        std::string hash_values;
        for (auto& reg : registers) {
            uint64_t hash = reg.get_hash();
            hash_values.append(reinterpret_cast<char*>(&hash), sizeof(hash));
        }
        return std::hash<std::string>{}(hash_values);
    }
};


Register Register::from_int(int64_t value) {
    // TODO: add your implementation here
    return Register{};
}


Register Register::from_string(const std::string& value) {
    // TODO: add your implementation here
    return Register{};
}


Register::Type Register::get_type() const {
    // TODO: add your implementation here
    return Type::INT64;
}


int64_t Register::as_int() const {
    // TODO: add your implementation here
    return 0;
}


std::string Register::as_string() const {
    // TODO: add your implementation here
    return std::string{};
}


uint64_t Register::get_hash() const {
    // TODO: add your implementation here
    return 0;
}


bool operator==(const Register& r1, const Register& r2) {
    // TODO: add your implementation here
    return false;
}


bool operator!=(const Register& r1, const Register& r2) {
    // TODO: add your implementation here
    return false;
}


bool operator<(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return false;
}


bool operator<=(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return false;
}


bool operator>(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return false;
}


bool operator>=(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return false;
}


Print::Print(Operator& input, std::ostream& stream) : UnaryOperator(input) {
    // TODO: add your implementation here
}


Print::~Print() = default;


void Print::open() {
    // TODO: add your implementation here
}


bool Print::next() {
    // TODO: add your implementation here
    return false;
}


void Print::close() {
    // TODO: add your implementation here
}


std::vector<Register*> Print::get_output() {
    // Print has no output
    return {};
}


Projection::Projection(Operator& input, std::vector<size_t> attr_indexes)
: UnaryOperator(input) {
    // TODO: add your implementation here
}


Projection::~Projection() = default;


void Projection::open() {
    // TODO: add your implementation here
}


bool Projection::next() {
    // TODO: add your implementation here
    return false;
}


void Projection::close() {
    // TODO: add your implementation here
}


std::vector<Register*> Projection::get_output() {
    // TODO: add your implementation here
    return {};
}


Select::Select(Operator& input, PredicateAttributeInt64 predicate)
: UnaryOperator(input) {
    // TODO: add your implementation here
}


Select::Select(Operator& input, PredicateAttributeChar16 predicate)
: UnaryOperator(input) {
    // TODO: add your implementation here
}


Select::Select(Operator& input, PredicateAttributeAttribute predicate)
: UnaryOperator(input) {
    // TODO: add your implementation here
}


Select::~Select() = default;


void Select::open() {
    // TODO: add your implementation here
}


bool Select::next() {
    // TODO: add your implementation here
    return false;
}


void Select::close() {
    // TODO: add your implementation here
}


std::vector<Register*> Select::get_output() {
    // TODO: add your implementation here
    return {};
}


Sort::Sort(Operator& input, std::vector<Criterion> criteria)
: UnaryOperator(input) {
    // TODO: add your implementation here
}


Sort::~Sort() = default;


void Sort::open() {
    // TODO: add your implementation here
}


bool Sort::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> Sort::get_output() {
    // TODO: add your implementation here
    return {};
}


void Sort::close() {
    // TODO: add your implementation here
}


HashJoin::HashJoin(
    Operator& input_left,
    Operator& input_right,
    size_t attr_index_left,
    size_t attr_index_right
) : BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


HashJoin::~HashJoin() = default;


void HashJoin::open() {
    // TODO: add your implementation here
}


bool HashJoin::next() {
    // TODO: add your implementation here
    return false;
}


void HashJoin::close() {
    // TODO: add your implementation here
}


std::vector<Register*> HashJoin::get_output() {
    // TODO: add your implementation here
    return {};
}


HashAggregation::HashAggregation(
    Operator& input,
    std::vector<size_t> group_by_attrs,
    std::vector<AggrFunc> aggr_funcs
) : UnaryOperator(input) {
    // TODO: add your implementation here
}


HashAggregation::~HashAggregation() = default;


void HashAggregation::open() {
    // TODO: add your implementation here
}


bool HashAggregation::next() {
    // TODO: add your implementation here
    return false;
};


void HashAggregation::close() {
    // TODO: add your implementation here
}


std::vector<Register*> HashAggregation::get_output() {
    // TODO: add your implementation here
    return {};
}


Union::Union(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


Union::~Union() = default;


void Union::open() {
    // TODO: add your implementation here
}


bool Union::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> Union::get_output() {
    // TODO: add your implementation here
    return {};
}


void Union::close() {
    // TODO: add your implementation here
}


UnionAll::UnionAll(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


UnionAll::~UnionAll() = default;


void UnionAll::open() {
    // TODO: add your implementation here
}


bool UnionAll::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> UnionAll::get_output() {
    // TODO: add your implementation here
    return {};
}


void UnionAll::close() {
    // TODO: add your implementation here
}


Intersect::Intersect(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


Intersect::~Intersect() = default;


void Intersect::open() {
    // TODO: add your implementation here
}


bool Intersect::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> Intersect::get_output() {
    // TODO: add your implementation here
    return {};
}


void Intersect::close() {
    // TODO: add your implementation here
}


IntersectAll::IntersectAll(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


IntersectAll::~IntersectAll() = default;


void IntersectAll::open() {
    // TODO: add your implementation here
}


bool IntersectAll::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> IntersectAll::get_output() {
    // TODO: add your implementation here
    return {};
}


void IntersectAll::close() {
    // TODO: add your implementation here
}


Except::Except(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


Except::~Except() = default;


void Except::open() {
    // TODO: add your implementation here
}


bool Except::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> Except::get_output() {
    // TODO: add your implementation here
    return {};
}


void Except::close() {
    // TODO: add your implementation here
}


ExceptAll::ExceptAll(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


ExceptAll::~ExceptAll() = default;


void ExceptAll::open() {
    // TODO: add your implementation here
}


bool ExceptAll::next() {
    // TODO: add your implementation here
    return false;
}


std::vector<Register*> ExceptAll::get_output() {
    // TODO: add your implementation here
    return {};
}


void ExceptAll::close() {
    // TODO: add your implementation here
}

}  // namespace moderndbs
