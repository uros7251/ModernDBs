#include "moderndbs/algebra.h"
#include <cassert>
#include <functional>
#include <string>
#include <cstring>

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
    Register reg {};
    reg.is_int = true;
    reg.integer = value;
    return reg;
}


Register Register::from_string(const std::string& value) {
    // TODO: add your implementation here
    if (value.size() > 16) throw std::runtime_error("The string consists of more than 16 characters!");
    Register reg {};
    reg.is_int = false;
    std::memset(reg.string, '\0', 16);
    std::memcpy(reg.string, value.data(), value.size());
    return reg;
}


Register::Type Register::get_type() const {
    // TODO: add your implementation here
    return is_int ? Type::INT64 : Type::CHAR16;
}


int64_t Register::as_int() const {
    // TODO: add your implementation here
    return integer;
}


std::string Register::as_string() const {
    // TODO: add your implementation here
    return string[15] ? std::string(string, 16) : std::string(string);
}


uint64_t Register::get_hash() const {
    // TODO: add your implementation here
    return is_int ? std::hash<uint64_t>()(integer) : std::hash<std::string>()(as_string());
}


bool operator==(const Register& r1, const Register& r2) {
    // TODO: add your implementation here
    return r1.is_int == r2.is_int && (
        r1.is_int ? r1.integer == r2.integer :
        !std::memcmp(r1.string, r2.string, 16));
}


bool operator!=(const Register& r1, const Register& r2) {
    // TODO: add your implementation here
    return !(r1==r2);
}


bool operator<(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return r1.is_int == r2.is_int && (
        r1.is_int ? r1.integer < r2.integer :
        std::memcmp(r1.string, r2.string, 16) < 0);
}


bool operator<=(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return r1.is_int == r2.is_int && (
        r1.is_int ? r1.integer <= r2.integer :
        std::memcmp(r1.string, r2.string, 16) <= 0);
}


bool operator>(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return !(r1<=r2);
}


bool operator>=(const Register& r1, const Register& r2) {
    assert(r1.get_type() == r2.get_type());
    // TODO: add your implementation here
    return !(r1<r2);
}


Print::Print(Operator& input, std::ostream& stream) : UnaryOperator(input), stream(stream) {
    // TODO: add your implementation here
}


Print::~Print() = default;


void Print::open() {
    // TODO: add your implementation here
    input->open();
}


bool Print::next() {
    // TODO: add your implementation here
    auto retval = input->next();
    if (retval) {
        auto tuple = input->get_output();
        bool first = true;
        for (auto* att: tuple) {
            if (!first) stream << ",";
            else first = !first;
            if (att->get_type() == Register::Type::INT64) stream << att->as_int();
            else stream << att->as_string();
        }
        stream << "\n";
    }
    return retval;
}


void Print::close() {
    // TODO: add your implementation here
    input->close();
}


std::vector<Register*> Print::get_output() {
    // Print has no output
    return {};
}


Projection::Projection(Operator& input, std::vector<size_t> attr_indexes)
: UnaryOperator(input), attr_indices(attr_indexes) {
    // TODO: add your implementation here
}


Projection::~Projection() = default;


void Projection::open() {
    // TODO: add your implementation here
    input->open();
    output.reserve(attr_indices.size());
    auto registers = input->get_output();
    for (auto i: attr_indices) {
        output.push_back(registers[i]);
    }
}


bool Projection::next() {
    // TODO: add your implementation here
    return input->next();
}


void Projection::close() {
    // TODO: add your implementation here
    output.clear();
    input->close();
}


std::vector<Register*> Projection::get_output() {
    // TODO: add your implementation here
    return output;
}

bool Select::filter_int(uint64_t left, uint64_t right, PredicateType predicate_type) {
    switch (predicate_type)
    {
    case PredicateType::EQ:
        return left == right;
    case PredicateType::GE:
        return left >= right;
    case PredicateType::GT:
        return left > right;
    case PredicateType::LE:
        return left <= right;
    case PredicateType::LT:
        return left < right;
    case PredicateType::NE:
        return left != right;
    default:
        throw std::runtime_error("Invalid predicate!");
    }
}

bool Select::filter_str(std::string left, std::string right, PredicateType predicate_type) {
    switch (predicate_type)
    {
    case PredicateType::EQ:
        return left == right;
    case PredicateType::GE:
        return left >= right;
    case PredicateType::GT:
        return left > right;
    case PredicateType::LE:
        return left <= right;
    case PredicateType::LT:
        return left < right;
    case PredicateType::NE:
        return left != right;
    default:
        throw std::runtime_error("Invalid predicate!");
    }
}

bool Select::filter_reg(const Register& left, const Register& right, PredicateType predicate_type) {
    switch (predicate_type)
    {
    case PredicateType::EQ:
        return left == right;
    case PredicateType::GE:
        return left >= right;
    case PredicateType::GT:
        return left > right;
    case PredicateType::LE:
        return left <= right;
    case PredicateType::LT:
        return left < right;
    case PredicateType::NE:
        return left != right;
    default:
        throw std::runtime_error("Invalid predicate!");
    }
}

Select::Select(Operator& input, PredicateAttributeInt64 predicate)
   : UnaryOperator(input), predicate(predicate) {
    // TODO: add your implementation here
}

Select::Select(Operator& input, PredicateAttributeChar16 predicate)
: UnaryOperator(input), predicate(predicate) {
    // TODO: add your implementation here
}


Select::Select(Operator& input, PredicateAttributeAttribute predicate)
: UnaryOperator(input), predicate(predicate) {
    // TODO: add your implementation here
}


Select::~Select() = default;


void Select::open() {
    // TODO: add your implementation here
    input->open();
    tuple = input->get_output();
}


bool Select::next() {
    // TODO: add your implementation here
    while (auto retval = input->next()) {
        auto variant = predicate.index();
        if (variant == 0) {
            auto p = std::get<PredicateAttributeInt64>(predicate);
            retval = filter_int(tuple[p.attr_index]->as_int(), p.constant, p.predicate_type);
        }
        else if (variant == 1) {
            auto p = std::get<PredicateAttributeChar16>(predicate);
            retval = filter_str(tuple[p.attr_index]->as_string(), p.constant, p.predicate_type);
        }
        else {
            auto p = std::get<PredicateAttributeAttribute>(predicate);
            retval = filter_reg(*tuple[p.attr_left_index], *tuple[p.attr_right_index], p.predicate_type);
        }
        if (retval) return true;
    }
    return false;
}


void Select::close() {
    // TODO: add your implementation here
    input->close();
}


std::vector<Register*> Select::get_output() {
    // TODO: add your implementation here
    return tuple;
}


Sort::Sort(Operator& input, std::vector<Criterion> criteria)
: UnaryOperator(input), criteria(criteria), next_index(0ul) {
    // TODO: add your implementation here
}


Sort::~Sort() = default;


void Sort::open() {
    // TODO: add your implementation here
    input->open();
    auto regs = input->get_output();
    output.resize(regs.size());
    while (input->next()) {
        std::vector<Register> tuple;
        for (auto* reg: regs) {
            tuple.emplace_back(*reg);
        }
        sorted_data.push_back(tuple);
    }
    std::sort(
        sorted_data.begin(),
        sorted_data.end(),
        [this](const std::vector<Register>& t1, const std::vector<Register>& t2) {
            for (auto& c: this->criteria) {
                if (t1[c.attr_index] == t2[c.attr_index]) continue;
                return c.desc ? t1[c.attr_index] > t2[c.attr_index] : t1[c.attr_index] < t2[c.attr_index];
            }
            return false;
        });
}


bool Sort::next() {
    // TODO: add your implementation here
    if (next_index++ >= sorted_data.size()) return false;
    auto& next_tuple = sorted_data[next_index-1];
    for (auto i=0ul; i < next_tuple.size(); ++i) {
        output[i] = next_tuple[i];
    }
    return true;
}


std::vector<Register*> Sort::get_output() {
    // TODO: add your implementation here
    std::vector<Register*> registers;
    registers.reserve(output.size());
    for (auto& attr: output) {
        registers.push_back(&attr);
    }
    return registers;
}


void Sort::close() {
    // TODO: add your implementation here
    input->close();
}


HashJoin::HashJoin(
    Operator& input_left,
    Operator& input_right,
    size_t attr_index_left,
    size_t attr_index_right
) : BinaryOperator(input_left, input_right), attr_index_left(attr_index_left), attr_index_right(attr_index_right) {
    // TODO: add your implementation here
}


HashJoin::~HashJoin() = default;


void HashJoin::open() {
    // TODO: add your implementation here
    input_left->open();
    auto regs = input_left->get_output();
    while (input_left->next()) {
        std::vector<Register> tuple;
        tuple.reserve(regs.size());
        for (auto* r: regs) {
            tuple.push_back(*r);
        }
        hash_map[*regs[attr_index_left]] = tuple;
    }
    output.resize(regs.size());
    input_right->open();
    right_registers = input_right->get_output();
}


bool HashJoin::next() {
    // TODO: add your implementation here
    while (input_right->next()) {
        if (hash_map.find(*right_registers[attr_index_right]) == hash_map.end()) continue;
        auto& left_tuple = hash_map[*right_registers[attr_index_right]];
        for (auto i=0ul; i<left_tuple.size(); ++i) {
            output[i] = left_tuple[i];
        }
        return true;
    }
    return false;
}


void HashJoin::close() {
    // TODO: add your implementation here
    input_left->close();
    input_right->close();
}


std::vector<Register*> HashJoin::get_output() {
    // TODO: add your implementation here
    std::vector<Register*> registers;
    registers.reserve(output.size()+right_registers.size());
    for (auto& attr: output) {
        registers.push_back(&attr);
    }
    for (auto* reg: right_registers) {
        registers.push_back(reg);
    }
    return registers;
}

void HashAggregation::aggregate(std::vector<Register> key, std::vector<Register> values) {
    auto& current_values = hash_map[key];
    for (auto i=0ul; i<aggr_funcs.size(); ++i) {
        switch (aggr_funcs[i].func)
        {
        case AggrFunc::Func::COUNT:
            current_values[i] = Register::from_int(current_values[i].as_int()+1ul);
            break;
        case AggrFunc::Func::MIN:
            if (values[i] < current_values[i]) current_values[i] = values[i];
            break;
        case AggrFunc::Func::MAX:
            if (values[i] > current_values[i]) current_values[i] = values[i];
            break;
        case AggrFunc::Func::SUM:
            assert(current_values[i].get_type() == Register::Type::INT64);
            current_values[i] = Register::from_int(current_values[i].as_int()+values[i].as_int());
            break;
        default:
            break;
        }
    }
}

HashAggregation::HashAggregation(
   Operator& input,
   std::vector<size_t>
      group_by_attrs,
   std::vector<AggrFunc>
      aggr_funcs) : UnaryOperator(input), group_by_attrs(group_by_attrs), aggr_funcs(aggr_funcs) {
    // TODO: add your implementation here
}

HashAggregation::~HashAggregation() = default;


void HashAggregation::open() {
    // TODO: add your implementation here
    input->open();
    auto regs = input->get_output();
    while (input->next()) {
        std::vector<Register> group_by_regs;
        for (auto attr: group_by_attrs) {
            group_by_regs.push_back(*regs[attr]);
        }
        std::vector<Register> values;
        values.reserve(aggr_funcs.size());
        for (auto& aggr_func: aggr_funcs) {
            values.push_back(
                aggr_func.func == AggrFunc::Func::COUNT ?
                Register::from_int(1ul) :
                *regs[aggr_func.attr_index]);
        }
        if (hash_map.find(group_by_regs) == hash_map.end()) {
            hash_map[group_by_regs] = values;
        }
        else {
            aggregate(group_by_regs, values);
        }
    }
    output.resize(group_by_attrs.size()+aggr_funcs.size());
}


bool HashAggregation::next() {
    // TODO: add your implementation here
    auto it = hash_map.begin();
    if (it == hash_map.end()) return false; 
    auto i=0ul;
    for (; i<it->first.size(); ++i) {
        output[i] = it->first[i];
    }
    for (auto j=0ul; i<output.size(); ++i, ++j) {
        output[i] = it->second[j];
    }
    hash_map.erase(it);
    return true;
};


void HashAggregation::close() {
    // TODO: add your implementation here
    input->close();
}


std::vector<Register*> HashAggregation::get_output() {
    // TODO: add your implementation here
    std::vector<Register*> registers;
    registers.reserve(output.size());
    for (auto& attr: output) {
        registers.push_back(&attr);
    }
    return registers;
}


Union::Union(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


Union::~Union() = default;


void Union::open() {
    // TODO: add your implementation here
    input_left->open();
    input_right->open();
    input = input_left->get_output();
    read_right = false;
    output.resize(input.size());
}


bool Union::next() {
    // TODO: add your implementation here
    while (true) {
        if (read_right && !input_right->next()) return false;
        else if (!read_right && !input_left->next()) {
            read_right = true;
            input = input_right->get_output();
            continue;
        }
        std::vector<Register> tuple;
        for (auto* attr: input) {
            tuple.push_back(*attr);
        }
        if (hash_set.find(tuple) == hash_set.end()) {
            for (auto i=0ul; i<output.size(); ++i) {
                output[i] = tuple[i];
            }
            hash_set.insert(tuple);
            return true;
        }
    }
}


std::vector<Register*> Union::get_output() {
    // TODO: add your implementation here
    std::vector<Register*> registers;
    registers.reserve(output.size());
    for (auto& attr: output) {
        registers.push_back(&attr);
    }
    return registers;
}


void Union::close() {
    // TODO: add your implementation here
    input_left->close();
    input_right->close();
}


UnionAll::UnionAll(Operator& input_left, Operator& input_right)
: BinaryOperator(input_left, input_right) {
    // TODO: add your implementation here
}


UnionAll::~UnionAll() = default;


void UnionAll::open() {
    // TODO: add your implementation here
    input_left->open();
    input_right->open();
    input = input_left->get_output();
    read_right = false;
    output.resize(input.size());
}


bool UnionAll::next() {
    // TODO: add your implementation here
    while (true) {
        if (read_right && !input_right->next()) return false;
        else if (!read_right && !input_left->next()) {
            read_right = true;
            input = input_right->get_output();
            continue;
        }
        for (auto i=0ul; i<output.size(); ++i) {
            output[i] = *input[i];
        }
        return true;
    }
}


std::vector<Register*> UnionAll::get_output() {
    // TODO: add your implementation here
    std::vector<Register*> registers;
    registers.reserve(output.size());
    for (auto& attr: output) {
        registers.push_back(&attr);
    }
    return registers;
}


void UnionAll::close() {
    // TODO: add your implementation here
    input_left->close();
    input_right->close();
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
