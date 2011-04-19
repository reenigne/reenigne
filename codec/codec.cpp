#include "unity/integer_types.h"
#include "unity/file.h"

static const int samplesPerSecond = 44100;
static const int changesPerSecond = 50;
static const int samplesPerChange = samplesPerSecond / changesPerSecond;
static const int totalChanges = 11275;    // 3 minutes 45.5 seconds
static const int totalChannels = 4;
static const int totalWaveforms = 0x100;
static const int samplesPerWaveform = 0x100;
static const int totalSamples = totalChanges * samplesPerChange;
static const int population = 100;

typedef SInt16 Sample;

Sample* original;

// Average of squared difference between corresponding samples - a very crude
// measure of how different two waveforms are.
// TODO: Use a more sophisticated psycho-acoustic metric.
//int distance(Sample* wave1, Sample* wave2)
//{
//    UInt64 total = 0;
//    for (int sample = 0; i < totalSamples; ++sample) {
//        Sample s1 = *wave1;
//        Sample s2 = *wave2;
//        ++wave1;
//        ++wave2;
//        int d = s1 - s2;
//        total += d*d;
//    }
//    return total/n;
//}

class Chromosome
{
public:
    Chromosome()
    {
        memset(_data, 0, sizeof(_data));
    }
    //void toWave(Sample* output)
    //{
    //    Channel channels[totalChannels];
    //    for (int change = 0; change < totalChanges; ++change) {
    //        for (int channel = 0; channel < totalChannels; ++channel)
    //            channels[channel].change(getChange(change, channel));
    //        for (int p = 0; p < totalChanges; ++p) {
    //            Sample s = 0;
    //            for (int channel = 0; channel < totalChannels; ++channel)
    //                s += channels[channel].sample(this);
    //            *output = s;
    //            ++output;
    //        }
    //    }
    //}
    int fitness()   // lower is better
    {
        UInt64 total = 0;
        Sample* o = original;
        Channel channels[totalChannels];
        for (int change = 0; change < totalChanges; ++change) {
            for (int channel = 0; channel < totalChannels; ++channel)
                channels[channel].change(getChange(change, channel));
            for (int p = 0; p < totalChanges; ++p) {
                Sample s = 0;
                for (int channel = 0; channel < totalChannels; ++channel)
                    s += channels[channel].sample(this);
                int d = (*o) - s;
                ++o;
                total += d*d;
            }
        }
        if (total == 0) {
            File file(String("hbfs.dat"));
            file.write(_data, sizeof(_data));
            printf("Lossless solution found!\n");
            exit(0);
        }
        return total / totalSamples;
    }

    void mutate()
    {
        _data[random(sizeof(Chomosome::_data))] ^= 1 << random(8);
    }
    void copyTo(Chromosome* chromosome)
    {
        memcpy(chromosome, this, sizeof(Chromosome));
    }

    class Change
    {
    public:
        UInt16 _velocity;
        UInt8 _volume;
        UInt8 _waveform;
    };

    class Channel
    {
    public:
        Channel() : _position(0) { }
        Sample sample(Chromosome* chromosome)
        {
            _position += _velocity;
            return (_volume*chromosome->waveData(_waveform, _position >> 8)) >> 10;
        }
        void change(Change* change)
        {
            _velocity = change->_velocity;
            _waveform = change->_waveform;
            _volume = change->_volume;
        }
    private:
        UInt16 _position;
        UInt16 _velocity;
        UInt8 _waveform;
        UInt8 _volume;
    };

    Sample waveData(int waveform, int position)
    {
        return *reinterpret_cast<Sample*>(_data + waveform*samplesPerWaveform + position);
    }
    Change* getChange(int change, int channel)
    {
        return *reinterpret_cast<Change*>(_data + totalWaveforms*samplesPerWaveform + (change*totalChannels + channel)*sizeof(Change));
    }

    UInt8 _data[totalWaveforms*samplesPerWaveform*sizeof(Sample) + totalChanges*totalChannels*sizeof(Change)];
};

void crossover(Chromosome* chromosome1, Chromosome* chromosome2)
{
    int i = random(sizeof(Chomosome::_data));
    for (int j = 0; j < i; ++j) {
        UInt8 t = chromosome1->_data[j];
        chromosome1->_data[j] = chromosome2->_data[j];
        chromosome2->_data[j] = t;
    }
}

Array<Chromosome> chromosomes;

#ifdef _WIN32
int main()
#else
int main(int argc, char* argv[])
#endif
{
    BEGIN_CHECKED {
        Array<Sample> hbfs;
        {
            File file(String("hbfs.wav"));
            file.seek(44);
            hbfs.allocate(totalSamples);
            original = &hbfs[0];
            file.read(original, totalSamples*sizeof(Sample));
        }
        chromosomes.allocate(population);
        int generation = 0;
        int fitnesses[population];
        int bestFitness = 0x7fffffff;
        for (int chromosome = 0; chromosome < population; ++chromosome) {
            fitnesses[chromosome] = chromosomes.fitness();
            if (fitnesses[chromosome] < bestFitness)
                bestFitness = fitnesses[chromosome];
        }
        do {
            // TODO: pick two individuals to die, weighted by fitness
            // TODO: pick two individuals to breed, weighted by fitness
            // TODO: replace the dead individuals with the children
            // TODO: mutate one of the children
            // TODO: recompute fitnesses for the children
            // TODO: update bestFitness
            // TODO: if generation % 1000 == 0 or if there's a new bestFitness:
            printf("Generation %i: fittest: %i\n", bestFitness);
        } while (true);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
