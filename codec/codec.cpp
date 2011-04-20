#include "unity/integer_types.h"
#include "unity/file.h"
#include "unity/random.h"
#include <stdio.h>
#include <math.h>

static const int samplesPerSecond = 44100;
static const int changesPerSecond = 50;
static const int samplesPerChange = samplesPerSecond / changesPerSecond;
static const int totalChanges = 11275;    // 3 minutes 45.5 seconds
static const int totalChannels = 4;
static const int totalWaveforms = 0x100;
static const int samplesPerWaveform = 0x100;
static const int totalSamples = totalChanges * samplesPerChange;
static const int population = 10;

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
    UInt64 noise()
    {
        UInt64 total = 0;
        Sample* o = original;
        Channel channels[totalChannels];
        for (int change = 0; change < totalChanges; ++change) {
            for (int channel = 0; channel < totalChannels; ++channel)
                channels[channel].change(getChange(change, channel));
            for (int p = 0; p < samplesPerChange; ++p) {
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
            FileHandle handle(file);
            handle.openWrite();
            handle.write(_data, sizeof(_data));
            printf("Lossless solution found!\n");
            exit(0);
        }
        return total;
    }

    void mutate()
    {
        _data[random(sizeof(_data))] ^= 1 << random(8);
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
            return (_volume*chromosome->waveData(_waveform, _position >> 8))
                >> 10;
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
        return *reinterpret_cast<Sample*>(_data + waveform*samplesPerWaveform +
            position);
    }
    Change* getChange(int change, int channel)
    {
        return reinterpret_cast<Change*>(_data +
            totalWaveforms*samplesPerWaveform +
            (change*totalChannels + channel)*sizeof(Change));
    }

    UInt8 _data[totalWaveforms*samplesPerWaveform*sizeof(Sample) +
        totalChanges*totalChannels*sizeof(Change)];
};

void crossover(Chromosome* chromosome1, Chromosome* chromosome2)
{
    int i = random(sizeof(chromosome1->_data));
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
            FileHandle handle(file);
            handle.openRead();
            handle.seek(44);
            hbfs.allocate(totalSamples);
            original = &hbfs[0];
            handle.read(original, totalSamples*sizeof(Sample));
        }
        chromosomes.allocate(population);
        int generation = 0;
        UInt64 noises[population];
        UInt64 quietness[population];
        bool chosen[population];
        UInt64 lowestNoise = 0x7fffffffffffffffLL;
        UInt64 totalNoise = 0;
        UInt64 totalQuiet = 0;
        int chromosome;
        for (chromosome = 0; chromosome < population; ++chromosome) {
            for (int i = 0; i < random(sizeof(chromosomes[0]._data)); ++i)
                chromosomes[chromosome]._data[i] = random(0x100);
            UInt64 noise = chromosomes[chromosome].noise();
            noises[chromosome] = noise;
            UInt64 quiet = static_cast<int>(0xffffffffffffffffLL / noise);
            quietness[chromosome] = quiet;
            lowestNoise = min(lowestNoise, noise);
            totalNoise += noise;
            totalQuiet += quiet;
            chosen[chromosome] = false;
        }
        do {
            for (chromosome = 0; chromosome < population; ++chromosome)
                chosen[chromosome] = false;

            SInt64 r = random64(totalNoise);
            for (chromosome = 0; chromosome < population; ++chromosome) {
                r -= noises[chromosome];
                if (r <= 0)
                    break;
            }
            int dies1 = chromosome;
            chosen[dies1] = true;
            totalNoise -= noises[dies1];
            totalQuiet -= quietness[dies1];

            r = random64(totalNoise);
            for (chromosome = 0; chromosome < population; ++chromosome) {
                if (chosen[chromosome])
                    continue;
                r -= noises[chromosome];
                if (r <= 0)
                    break;
            }
            int dies2 = chromosome;
            chosen[dies2] = true;
            totalNoise -= noises[dies2];
            totalQuiet -= quietness[dies2];

            r = random64(totalQuiet);
            for (chromosome = 0; chromosome < population; ++chromosome) {
                if (chosen[chromosome])
                    continue;
                r -= quietness[chromosome];
                if (r <= 0)
                    break;
            }
            int parent1 = chromosome;
            chosen[parent1] = true;
            
            r = random64(totalQuiet - quietness[parent1]);
            for (chromosome = 0; chromosome < population; ++chromosome) {
                if (chosen[chromosome])
                    continue;
                r -= quietness[chromosome];
                if (r <= 0)
                    break;
            }
            int parent2 = chromosome;
            //printf("%i+%i->%i+%i\n",parent1,parent2,dies1,dies2);
            
            chosen[dies1] = false;
            chosen[dies2] = false;
            chosen[parent1] = false;

            chromosomes[parent1].copyTo(&chromosomes[dies1]);
            chromosomes[parent2].copyTo(&chromosomes[dies2]);
            crossover(&chromosomes[dies1], &chromosomes[dies2]);
            chromosomes[dies1].mutate();
            bool newLowestNoise = false;
            UInt64 noise = chromosomes[dies1].noise();
            if (noise < lowestNoise) {
                lowestNoise = noise;
                newLowestNoise = true;
            }
            noises[dies1] = noise;
            UInt64 quiet = static_cast<int>(0xffffffffffffffffLL / noise);
            quietness[dies1] = quiet;
            totalNoise += noise;
            totalQuiet += quiet;
            noise = chromosomes[dies2].noise();
            if (noise < lowestNoise) {
                lowestNoise = noise;
                newLowestNoise = true;
            }
            noises[dies2] = noise;
            quiet = static_cast<int>(0xffffffffffffffffLL / noise);
            quietness[dies2] = quiet;
            totalNoise += noise;
            totalQuiet += quiet;

            if (generation % 1000 == 0 || newLowestNoise)
                printf("Generation %i: fittest: %lf\n", generation, sqrt(static_cast<double>(lowestNoise)/static_cast<double>(totalSamples)));
            ++generation;

            if (generation % 1000 == 0) {
                UInt64 totalNoise2 = 0;
                UInt64 totalQuiet2 = 0;
                for (chromosome = 0; chromosome < population; ++chromosome) {
                    UInt64 noise = chromosomes[chromosome].noise();
                    if (noises[chromosome] != noise)
                        printf("noise recorded incorrectly\n");
                    UInt64 quiet = static_cast<int>(0xffffffffffffffffLL / noise);
                    if (quietness[chromosome] != quiet)
                        printf("quiet recorded incorrectly\n");
                    totalNoise2 += noise;
                    totalQuiet2 += quiet;
                }
                if (totalNoise != totalNoise2)
                    printf("totalNoise recorded incorrectly\n");
                if (totalQuiet != totalQuiet2)
                    printf("totalQuiet recorded incorrectly\n");
            }

        } while (true);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}