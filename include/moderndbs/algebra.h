#ifndef INCLUDE_MODERNDBS_ALGEBRA_H
#define INCLUDE_MODERNDBS_ALGEBRA_H

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>


namespace moderndbs::iterator_model {

/// A register represents an arbitrary scalar value
class Register {
private:
    // TODO: add your implementation here

public:
    /// The underlying type of the register. We simplify the implementation
    /// by only supporting fixed-sized int64_t or strings of size 16
    enum class Type { INT64, CHAR16 };

    Register() = default;
    ~Register() = default;
    Register(const Register&) = default;
    Register(Register&&) = default;

    Register& operator=(const Register&) = default;
    Register& operator=(Register&&) = default;

    /// Creates a `Register` from a given `int64_t`.
    static Register from_int(int64_t value);

    /// Creates a `Register` from a given `std::string`. The register must only
    /// be able to hold fixed size strings of size 16, so `value` must be at
    /// least 16 characters long.
    static Register from_string(const std::string& value);

    /// Returns the type of the register.
    [[nodiscard]] Type get_type() const;

    /// Returns the `int64_t` value for this register. Must only be called when
    /// this register really is an integer.
    [[nodiscard]] int64_t as_int() const;

    /// Returns the `std::string` value for this register. Must only be called
    /// when this register really is a string.
    [[nodiscard]] std::string as_string() const;

    /// Returns the hash value for this register.
    [[nodiscard]] uint64_t get_hash() const;

    /// Compares two register for equality.
    friend bool operator==(const Register& r1, const Register& r2);

    /// Compares two registers for inequality.
    friend bool operator!=(const Register& r1, const Register& r2);

    /// Compares two registers for `<`. Must only be called when `r1` and `r2`
    /// have the same type.
    friend bool operator<(const Register& r1, const Register& r2);

    /// Compares two registers for `<=`. Must only be called when `r1` and `r2`
    /// have the same type.
    friend bool operator<=(const Register& r1, const Register& r2);

    /// Compares two registers for `>`. Must only be called when `r1` and `r2`
    /// have the same type.
    friend bool operator>(const Register& r1, const Register& r2);

    /// Compares two registers for `>=`. Must only be called when `r1` and `r2`
    /// have the same type.
    friend bool operator>=(const Register& r1, const Register& r2);
};

/// An abstract operator that we can 
class Operator {
public:
    Operator() = default;
    Operator(const Operator&) = delete;
    Operator(Operator&&) = delete;
    Operator& operator=(const Operator&) = delete;
    Operator& operator=(Operator&&) = delete;

    virtual ~Operator() = default;

    /// Initializes the operator.
    virtual void open() = 0;

    /// Tries to generate the next tuple. Return true when a new tuple is
    /// available. The values will be passed out-of-line, see @get_output().
    virtual bool next() = 0;

    /// Destroys the operator.
    virtual void close() = 0;

    /// This returns the pointers to the registers of the generated tuple. When
    /// `next()` returns true, the Registers will contain the values for the
    /// next tuple. Each `Register*` in the vector stands for one attribute of
    /// the tuple.
    /// The registers can either be stored in each (source) operator, or in
    /// a buffer shared by all operators.
    virtual std::vector<Register*> get_output() = 0;
};


class UnaryOperator
: public Operator {
protected:
    Operator* input;

public:
    explicit UnaryOperator(Operator& input) : input(&input) {}
    UnaryOperator(const UnaryOperator&) = delete;
    UnaryOperator(UnaryOperator&&) = delete;
    UnaryOperator& operator=(const UnaryOperator&) = delete;
    UnaryOperator& operator=(UnaryOperator&&) = delete;
    ~UnaryOperator() override = default;
};


class BinaryOperator
: public Operator {
protected:
    Operator* input_left;
    Operator* input_right;

public:
    explicit BinaryOperator(Operator& input_left, Operator& input_right)
    : input_left(&input_left), input_right(&input_right) {}

    BinaryOperator(const BinaryOperator&) = delete;
    BinaryOperator(BinaryOperator&&) = delete;
    BinaryOperator& operator=(const BinaryOperator&) = delete;
    BinaryOperator& operator=(BinaryOperator&&) = delete;
    ~BinaryOperator() override = default;
};


/// Prints all tuples from its input into the stream. Tuples are separated by a
/// newline character ("\n") and attributes are separated by a single comma
/// without any extra spaces. The last line also ends with a newline. Calling
/// `next()` prints the next tuple.
class Print
: public UnaryOperator {
private:
    // TODO: add your implementation here

public:
    Print(Operator& input, std::ostream& stream);

    Print(const Print&) = delete;
    Print(Print&&) = delete;
    Print& operator=(const Print&) = delete;
    Print& operator=(Print&&) = delete;
    ~Print() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Generates tuples from the input with only a subset of their attributes.
class Projection
: public UnaryOperator {
private:
    // TODO: add your implementation here

public:
    Projection(Operator& input, std::vector<size_t> attr_indexes);
    Projection(const Projection&) = delete;
    Projection(Projection&&) = delete;
    Projection& operator=(const Projection&) = delete;
    Projection& operator=(Projection&&) = delete;
    ~Projection() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Filters tuples with the given predicate.
class Select
: public UnaryOperator {
public:
    enum class PredicateType {
        EQ, // a == b
        NE, // a != b
        LT, // a < b
        LE, // a <= b
        GT, // a > b
        GE  // a >= b
    };

    /// Predicate of the form:
    /// tuple[attr_index] P constant
    /// where P is given by `predicate_type`.
    struct PredicateAttributeInt64 {
        size_t attr_index;
        int64_t constant;
        PredicateType predicate_type;
    };

    /// Predicate of the form:
    /// tuple[attr_index] P constant
    /// where P is given by `predicate_type` and `constant` is a string of
    /// length 16.
    struct PredicateAttributeChar16 {
        size_t attr_index;
        std::string constant;
        PredicateType predicate_type;
    };

    /// tuple[attr_left_index] P tuple[attr_right_index]
    /// where P is given by `predicate_type`.
    struct PredicateAttributeAttribute {
        size_t attr_left_index;
        size_t attr_right_index;
        PredicateType predicate_type;
    };

private:
    // TODO: add your implementation here

public:
    Select(Operator& input, PredicateAttributeInt64 predicate);
    Select(Operator& input, PredicateAttributeChar16 predicate);
    Select(Operator& input, PredicateAttributeAttribute predicate);

    Select(const Select&) = delete;
    Select(Select&&) = delete;
    Select& operator=(const Select&) = delete;
    Select& operator=(Select&&) = delete;
    ~Select() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Sorts the input by the given criteria.
class Sort
: public UnaryOperator {
public:
    /// A sort criterion. This represents one part of the specified order:
    /// ORDER BY column1 [ASC | DESC] [, column2 [ASC | DESC] ...]
    struct Criterion {
        /// Attribute to be sorted as an index of the input.
        size_t attr_index;
        /// Sort descending?
        bool desc;
    };

private:
    // TODO: add your implementation here

public:
    Sort(Operator& input, std::vector<Criterion> criteria);

    Sort(const Sort&) = delete;
    Sort(Sort&&) = delete;
    Sort& operator=(const Sort&) = delete;
    Sort& operator=(Sort&&) = delete;
    ~Sort() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes the inner equi-join of the two inputs on one attribute.
/// The general idea is to build a hash-table (e.g., std::unordered_map) on 
/// the left input, then probe with the right input, and output the matching 
/// combined all tuple.
/// This is a simplified version that only needs to support unique left keys
/// (i.e., no multimap).
class HashJoin
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    /// Constructor
    /// attr_index_left: the left key to compare as an index from input_left's output
    /// attr_index_right: the right key to compare as an index from input_right's output
    HashJoin(
        Operator& input_left,
        Operator& input_right,
        size_t attr_index_left,
        size_t attr_index_right
    );

    HashJoin(const HashJoin&) = delete;
    HashJoin(HashJoin&&) = delete;
    HashJoin& operator=(const HashJoin&) = delete;
    HashJoin& operator=(HashJoin&&) = delete;
    ~HashJoin() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Groups and calculates (potentially multiple) aggregates on the input.
/// Aggregates the input in a hash-table (e.g., std::unordered_map), and 
/// outputs the completed aggregates in any order.
/// Note that we can have multiple keys and multiple aggregates!
class HashAggregation
: public UnaryOperator {
public:
    /// Represents an aggregation function. For MIN, MAX, and SUM `attr_index`
    /// stands for the attribute which is being aggregated. For SUM the
    /// attribute must be in an `INT64` register.
    struct AggrFunc {
        enum Func { MIN, MAX, SUM, COUNT };

        Func func;
        size_t attr_index;
    };

private:
    // TODO: add your implementation here

public:
    /// Constructor
    /// group_by_attrs: the input keys to build groups over as an index from input's output
    /// aggr_funcs: aggregations to compute, referencing attributes as an index from input's output
    HashAggregation(
        Operator& input,
        std::vector<size_t> group_by_attrs,
        std::vector<AggrFunc> aggr_funcs
    );

    HashAggregation(const HashAggregation&) = delete;
    HashAggregation(HashAggregation&&) = delete;
    HashAggregation& operator=(const HashAggregation&) = delete;
    HashAggregation& operator=(HashAggregation&&) = delete;
    ~HashAggregation() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes the union of the two inputs with set semantics (removing duplicates).
/// This can be computed, using a hash set (e.g., std::unordered_set) that compares
/// all attributes of the inpout.
class Union
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    Union(Operator& input_left, Operator& input_right);

    Union(const Union&) = delete;
    Union(Union&&) = delete;
    Union& operator=(const Union&) = delete;
    Union& operator=(Union&&) = delete;
    ~Union() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes the union of the two inputs with bag semantics.
class UnionAll
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    UnionAll(Operator& input_left, Operator& input_right);

    UnionAll(const UnionAll&) = delete;
    UnionAll(UnionAll&&) = delete;
    UnionAll& operator=(const UnionAll&) = delete;
    UnionAll& operator=(UnionAll&&) = delete;
    ~UnionAll() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes the intersection of the two inputs with set semantics. A possible
/// implementation can use a hash set (e.g., std::unordered_set).
/// This is an optional assignment
class Intersect
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    Intersect(Operator& input_left, Operator& input_right);

    Intersect(const Intersect&) = delete;
    Intersect(Intersect&&) = delete;
    Intersect& operator=(const Intersect&) = delete;
    Intersect& operator=(Intersect&&) = delete;
    ~Intersect() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes the intersection of the two inputs with bag semantics. A possible
/// implementation can use a hash map with a counter (e.g., std::unordered_map).
/// This is an optional assignment
class IntersectAll
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    IntersectAll(Operator& input_left, Operator& input_right);

    IntersectAll(const IntersectAll&) = delete;
    IntersectAll(IntersectAll&&) = delete;
    IntersectAll& operator=(const IntersectAll&) = delete;
    IntersectAll& operator=(IntersectAll&&) = delete;
    ~IntersectAll() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes input_left - input_right with set semantics. A possible
/// implementation can use a hash set (e.g., std::unordered_set).
/// This is an optional assignment
class Except
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    Except(Operator& input_left, Operator& input_right);

    Except(const Except&) = delete;
    Except(Except&&) = delete;
    Except& operator=(const Except&) = delete;
    Except& operator=(Except&&) = delete;
    ~Except() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};


/// Computes input_left - input_right with bag semantics. A possible
/// implementation can use a hash map with a counter (e.g., std::unordered_map).
/// This is an optional assignment
class ExceptAll
: public BinaryOperator {
private:
    // TODO: add your implementation here

public:
    ExceptAll(Operator& input_left, Operator& input_right);

    ExceptAll(const ExceptAll&) = delete;
    ExceptAll(ExceptAll&&) = delete;
    ExceptAll& operator=(const ExceptAll&) = delete;
    ExceptAll& operator=(ExceptAll&&) = delete;
    ~ExceptAll() override;

    void open() override;
    bool next() override;
    void close() override;
    std::vector<Register*> get_output() override;
};

}  // namespace moderndbs

#endif
