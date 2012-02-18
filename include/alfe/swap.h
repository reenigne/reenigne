#ifndef INCLUDED_SWAP_H
#define INCLUDED_SWAP_H

template<class T> void swap(T& x, T& y) { T z = x; x = y; y = z; }

#endif // INCLUDED_SWAP_H
