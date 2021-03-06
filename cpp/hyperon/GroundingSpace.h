#ifndef GROUNDING_SPACE_H
#define GROUNDING_SPACE_H

#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <memory>
#include <map>

#include "SpaceAPI.h"

// Atom

class Atom;

using AtomPtr = std::shared_ptr<Atom>;

class Atom {
public:
    enum Type {
        SYMBOL,
        GROUNDED,
        EXPR,
        VARIABLE
    };

    static AtomPtr INVALID;

    virtual ~Atom() { }
    virtual Type get_type() const = 0;
    virtual bool operator==(Atom const& other) const = 0;
    virtual bool operator!=(Atom const& other) const { return !(*this == other); }
    virtual std::string to_string() const = 0;
};

std::string to_string(Atom::Type type);
bool operator==(std::vector<AtomPtr> const& a, std::vector<AtomPtr> const& b); 
std::string to_string(std::vector<AtomPtr> const& atoms, std::string delimiter);

// Symbol atom

class SymbolAtom : public Atom {
public:
    SymbolAtom(std::string symbol) : symbol(symbol) { }
    virtual ~SymbolAtom() { }
    std::string get_symbol() const { return symbol; }

    Type get_type() const override { return SYMBOL; }
    bool operator==(Atom const& _other) const override { 
        SymbolAtom const* other = dynamic_cast<SymbolAtom const*>(&_other);
        return other && symbol == other->symbol;
    }
    std::string to_string() const override { return symbol; }
private:
    std::string symbol;
};

using SymbolAtomPtr = std::shared_ptr<SymbolAtom>;

inline auto S(std::string symbol) {
    return std::make_shared<SymbolAtom>(symbol);
}

// Expression atom

class ExprAtom : public Atom {
public:
    ExprAtom(std::initializer_list<AtomPtr> children) : children(children) { }
    ExprAtom(std::vector<AtomPtr> children) : children(children) { }
    virtual ~ExprAtom() { }
    std::vector<AtomPtr>& get_children() { return children; }

    Type get_type() const override { return EXPR; }
    bool operator==(Atom const& _other) const override;
    std::string to_string() const override { return "(" + ::to_string(children, " ") + ")"; }

private:
    std::vector<AtomPtr> children;
};

using ExprAtomPtr = std::shared_ptr<ExprAtom>;

inline auto E(std::initializer_list<AtomPtr> children) {
    return std::make_shared<ExprAtom>(children);
}

inline auto E(std::vector<AtomPtr> children) {
    return std::make_shared<ExprAtom>(children);
}

// Variable atom

class VariableAtom : public Atom {
public:
    VariableAtom(std::string name) : name(name) { }
    virtual ~VariableAtom() { }
    std::string get_name() const { return name; }

    Type get_type() const override { return VARIABLE; }
    bool operator==(Atom const& _other) const override {
        VariableAtom const* other = dynamic_cast<VariableAtom const*>(&_other);
        return other && name == other->name;
    }
    std::string to_string() const override { return "$" + name; }
private:
    std::string name;
};

using VariableAtomPtr = std::shared_ptr<VariableAtom>;

inline auto V(std::string name) {
    return std::make_shared<VariableAtom>(name);
}

class LessVariableAtomPtr {
public:
    bool operator()(VariableAtomPtr const& a, VariableAtomPtr const& b) const {
        return a->get_name() < b->get_name();
    }
};

using Bindings = std::map<VariableAtomPtr, AtomPtr, LessVariableAtomPtr>;

// Grounded atom

class GroundingSpace;

class GroundedAtom : public Atom {
public:
    virtual ~GroundedAtom() { }
    virtual void execute(GroundingSpace const& args, GroundingSpace& result) const {
        throw std::runtime_error("Operation is not supported");
    }

    Type get_type() const override { return GROUNDED; }
};

using GroundedAtomPtr = std::shared_ptr<GroundedAtom>;

template <typename T>
class ValueAtom : public GroundedAtom {
public:
    ValueAtom(T value) : value(value) { }
    virtual ~ValueAtom() { }
    bool operator==(Atom const& _other) const override { 
        // TODO: should it be replaced by type checking?
        ValueAtom const* other = dynamic_cast<ValueAtom const*>(&_other);
        return other && other->value == value;
    }
    T get() const { return value; }
private:
    T value;
};

// Space

struct Unification {
    Unification(AtomPtr a, AtomPtr b) : a(a), b(b) {}
    AtomPtr a;
    AtomPtr b;
};

using Unifications = std::vector<Unification>;

struct UnificationResult {
    // FIXME: a_bindings can be removed from here
    Bindings a_bindings;
    Bindings b_bindings;
    Unifications unifications;
};

class GroundingSpace : public SpaceAPI {
public:

    static std::string TYPE;

    GroundingSpace() { }
    GroundingSpace(std::initializer_list<AtomPtr> content) : content(content) { }
    GroundingSpace(std::vector<AtomPtr> content) : content(content) { }

    virtual ~GroundingSpace() { }

    void add_native(const SpaceAPI* other) override {
        throw std::logic_error("Method is not implemented");
    }

    std::string get_type() const override { return TYPE; }

    void add_atom(AtomPtr atom) {
        content.push_back(atom);
    }

    // TODO: Which operations should we add into SpaceAPI to make
    // interpret_step space implementation agnostic?
    // If GroundedAtom will be cross-space interface and its execute method
    // will input and return SpaceAPI then interpret_step could be implemented
    // on a SpaceAPI level.
    AtomPtr interpret_step(SpaceAPI const& kb);
    // TODO: Discuss moving into SpaceAPI as match_to replacement
    std::vector<Bindings> match(AtomPtr pattern) const;
    // FIXME: this method can be removed and implemented in client code on top
    // of GroundingSpace::match
    void match(SpaceAPI const& pattern, SpaceAPI const& templ, GroundingSpace& space) const;
    std::vector<UnificationResult> unify(AtomPtr atom) const;
    std::vector<AtomPtr> const& get_content() const { return content; }

    bool operator==(SpaceAPI const& space) const;
    bool operator!=(SpaceAPI const& other) const { return !(*this == other); }
    std::string to_string() const { return "<" + ::to_string(content, ", ") + ">"; }

private:

    std::vector<AtomPtr> content;
};

// TODO: think how to export it properly: either we should export API to
// implement it and move it into common library or we should consider it to be
// a part of the GroundingSpace API
extern const GroundedAtomPtr IFMATCH;

#endif // GROUNDING_SPACE_H
