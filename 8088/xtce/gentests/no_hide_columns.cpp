Between the lines "line = 0;" and "File("expected.txt").openWrite().write(expected1);" use the following code to print all columns
In test "{{  {0x00, 0x04, 0x0000, 0x00000000}}, 0}", the next prefetch address is briefly placed on the CPU's bus during the idle period before the synchronous fetch, before being replaced by the synchronous address


                    do {
                        int ec = e.get();
                        int oc = s.get();
                        if (ec == '?' && oc != -1)
                            oc = '?';
                        ++column;
                        if ((column >= 7 && column < 20) || column >= 23 ||
                            !hideColumns) {
                            if (line < c) {
                                observed += codePoint(oc);
                                expected1 += codePoint(ec);
                            }
                        }
                        if (ec == '\n') {
                            ++line;
                            column = 1;
                        }
                        if (ec != oc || ec == -1)
                            break;
                    } while (true);

                    CharacterSource s2(s);
                    CharacterSource oldS2(s2);
                    do {
                        if (parse(&s2, "Program ended normally."))
                            break;
                        int c = s2.get();
                        if (c == -1)
                            throw Exception("runtests didn't end properly");
                        oldS2 = s2;
                    } while (true);
                    if (line < c) {
                        expected1 += e.subString(e.offset(), e.length());
                        observed += s.subString(s.offset(), oldS2.offset());
                    }

