// Update counts

int w = _waveform.count();
for (int i = 0; i < 1000; ++i) {
    int wp = random(w);
    int x = (wp*_size.x)/w;
    int y = ((_waveform[p] + 0x8000)*_size.y) >> 16;
    p = y*_size.x + x;
    ++_counts[p];
    if (_counts[p] > _maximum)
        ++_maximum;
}


// Init exposure array
if (_exposures.count() < _maximum)
_exposures.


// Convert to exposure

Byte* line = _buffer;
int countP = 0;
for (int y = 0; y < _size.y; ++y) {
    Byte* p = line;
    for (int x = 0; x < _size.x; ++x) {
        int c = _counts[countP];
        ++countP;
        // TODO: Convert c to 0..767
        c = static_cast<int>(767.0f*exp(
    }
    line += _stride;
}

Black->Blue->Cyan->White
Black->Blue->Magenta->White
Black->Red->Magenta->White
Black->Red->Yellow->White
Black->Green->Yellow->White
Black->Green->Cyan->White
