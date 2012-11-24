class VTable
{
};

class Raviolo
{
public:
    VTable* vTable;
    Raviolo* a;
    Raviolo* b;
    Raviolo** pp;
};

class NormalRaviolo : public Raviolo
{
};
