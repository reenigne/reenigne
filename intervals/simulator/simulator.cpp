#include "file.h"
#include "string.h"
#include "exception.h"
#include "handle.h"

int main()
{
	BEGIN_CHECKED {
		String fileName("../intervals.HEX");
		File file(fileName);
		String contents = file.contents();
		contents.write(Handle::consoleOutput());
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}