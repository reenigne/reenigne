class Edge;

class EdgeExpression
{
public:
    EdgeExpression operator+(EdgeExpression a)
    {
        EdgeExpression l(*this);
        for (auto p : a._clauses)
            l.add(*p);
        return l;
    }
    EdgeExpression operator-(EdgeExpression a)
    {
        EdgeExpression l(*this);
        for (auto p : a._clauses) {
            Clause c = *p;
            c._coefficient = -c._coefficient;
            l.add(c);
        }
        return l;
    }
    EdgeExpression operator*(double a)
    {
        EdgeExpression l(*this);
        l.multiply(a);
        return l;
    }
    EdgeExpression operator/(double a)
    {
        EdgeExpression l(*this);
        l.multiply(1.0/a);
        return l;
    }
private:
    class Clause
    {
    public:
        Clause(Edge* edge, double coefficient)
          : _edge(edge), _coefficient(coefficient) { }
        Edge* _edge;
        double _coefficient;
    };
    List<Clause> _clauses;
    double _offset;

    void setToEdge(Edge* edge)
    {
        _clauses = List<Clause>();
        _clauses.add(Clause(edge, 1));
        _offset = 0;
    }
    void add(const Clause& clause)
    {
        for (auto p = _clauses)
            if (clause._edge == p._edge) {
                p._coefficient += clause._coefficient;
                return;
            }
        _clauses.add(clause);
    }
    void add(double offset)
    {
        _offset += offset;
    }
    void multiply(double a)
    {
        for (auto p : _clauses)
            p._coefficient *= a;
    }

    friend class Edge;
};

class Edge : public EdgeExpression
{
public:
    Edge()
    {
        setToEdge(this);
    }
    void operator=(const EdgeExpression& expression)
    {
        EdgeExpression::operator=(expression);
    }
private:
    List<Edge> _dependents;
};



// Add to Window:
//    Edge top;
//    Edge left;
//    Edge right;
//    Edge bottom;


