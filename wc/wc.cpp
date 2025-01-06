#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/hash_table.h"
#include "alfe/julian.h"

class Program : public ProgramBase
{
public:
    bool haveStart;
    int words;
    HashTable<String, double> averages;
    String author;
    int julian = 0;
    int minute, hour, day, month, year;

    void doOutput()
    {
        if (!haveStart)
            return;
        //console.write(decimal(year, 4) + "/" +
        //    decimal(month, 2) + "/" + decimal(day, 2) +
        //    " " + decimal(hour, 2) + ":" +
        //    decimal(minute, 2) + ", " + author + ", " +
        //    decimal(words) + "\n");
        if (!averages.hasKey(author))
            averages.add(author, 0);
        int newJulian = greg2jul(day, month, year);
        if (julian == 0)
            julian = newJulian;
        bool outputted = false;
        while (julian != newJulian) {
            if (!outputted) {
                printf("%4i/%2i/%2i, ", year, month, day);
                bool first = true;
                for (auto kv : averages) {
                    if (!first)
                        printf(", ");
                    printf("%f", kv.value());
                    first = false;
                }
                printf("\n");
                outputted = true;
            }
            for (auto kv : averages)
                averages[kv.key()] = kv.value() * 0.8;
            ++julian;
        }
        averages[author] += words;
        words = 0;
    }
    void run()
    {
        String contents = File(_arguments[1], true).contents();
        CharacterSource s(contents);
        bool startOfLine = true;
        words = 0;
        haveStart = false;
        bool inWord = false;
        do {
            CharacterSource s2 = s;
            int c = s.getByte();
            if (startOfLine) {
                if (c == '[') {
                    CharacterSource s3 = s;
                    int c2 = s.get();
                    if (c2 >= '0' && c2 <= '9') {
                        doOutput();
                        haveStart = true;

                        s = s3;
                        Rational hourR;
                        Space::parseNumber(&s, &hourR);
                        hour = hourR.floor();
                        if (hour == 12)
                            hour = 0;

                        s.assert(':');

                        Rational minuteR;
                        Space::parseNumber(&s, &minuteR);
                        minute = minuteR.floor();

                        //s.assert(' ');
                        c2 = s.get();
                        if (c2 == 'p' || c2 == 'P')
                            hour += 12;
                        else {
                            if (c2 != 'a' && c2 != 'A')
                                goto abortParse;
                                
                        }
                        s.get(); // s.assert('m');
                        s.assert(',');
                        s.assert(' ');

                        Rational dayR;
                        Space::parseNumber(&s, &dayR);
                        day = dayR.floor();

                        s.assert('/');

                        Rational monthR;
                        Space::parseNumber(&s, &monthR);
                        month = monthR.floor();
                        if (c2 == 'P') {
                            int t = month;
                            month = day;
                            day = t;
                        }

                        s.assert('/');

                        Rational yearR;
                        Space::parseNumber(&s, &yearR);
                        year = yearR.floor();

                        s.assert(']');
                        s.assert(' ');
                        int start = s.offset();
                        do {
                            c2 = s.get();
                            if (c2 == ':')
                                break;
                        } while (true);
                        int end = s.offset();
                        author = contents.subString(start, end - start);

                        s2 = s;
                    }
                }
            }
        abortParse:
            c = s2.getByte();
            bool newInWord = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '\'' || c >= 0x80);
            if (newInWord && !inWord)
                ++words;
            inWord = newInWord;
            if (c == -1) {
                doOutput();
                break;
            }
            startOfLine = (c == 10 || c == 13);
        } while (true);
    }
};