#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/colour_space.h"
#include "alfe/user.h"
#include "alfe/cga.h"
#include "alfe/config_file.h"

Array<Byte> tileMap;
Array<Byte> tileData;
Array<Byte> skybox;

class CGAKartWindow : public RootWindow
{
public:
	void setOutput(CGAOutput* output) { _output = output; }
	void setConfig(ConfigFile* configFile, File configPath)
	{
		_configFile = configFile;
		_sequencer.setROM(
			File(configFile->get<String>("cgaROM"), configPath.parent()));

		_output->setConnector(1);          // old composite
		_output->setScanlineProfile(0);    // rectangle
		_output->setHorizontalProfile(0);  // rectangle
		_output->setScanlineWidth(1);
		_output->setScanlineBleeding(2);   // symmetrical
		_output->setHorizontalBleeding(2); // symmetrical
		_output->setZoom(2);
		_output->setHorizontalRollOff(0);
		_output->setHorizontalLobes(4);
		_output->setVerticalRollOff(0);
		_output->setVerticalLobes(4);
		_output->setSubPixelSeparation(1);
		_output->setPhosphor(0);           // colour
		_output->setMask(0);
		_output->setMaskSize(0);
		_output->setAspectRatio(5.0/6.0);
		_output->setOverscan(0.1);
		_output->setCombFilter(0);         // no filter
		_output->setHue(0);
		_output->setSaturation(100);
		_output->setContrast(100);
		_output->setBrightness(0);
		_output->setShowClipping(false);
		_output->setChromaBandwidth(1);
		_output->setLumaBandwidth(1);
		_output->setRollOff(0);
		_output->setLobes(1.5);
		_output->setPhase(1);

		_regs = -CGAData::registerLogCharactersPerBank;
		_cgaBytes.allocate(0x4000 + _regs);
		_vram = &_cgaBytes[_regs];
		_vram[CGAData::registerLogCharactersPerBank] = 12;
		_vram[CGAData::registerScanlinesRepeat] = 1;
		_vram[CGAData::registerHorizontalTotalHigh] = 0;
		_vram[CGAData::registerHorizontalDisplayedHigh] = 0;
		_vram[CGAData::registerHorizontalSyncPositionHigh] = 0;
		_vram[CGAData::registerVerticalTotalHigh] = 0;
		_vram[CGAData::registerVerticalDisplayedHigh] = 0;
		_vram[CGAData::registerVerticalSyncPositionHigh] = 0;
		_vram[CGAData::registerMode] = 0x1a;
		_vram[CGAData::registerPalette] = 0x0f;
		_vram[CGAData::registerHorizontalTotal] = 57 - 1;
		_vram[CGAData::registerHorizontalDisplayed] = 40;
		_vram[CGAData::registerHorizontalSyncPosition] = 45;
		_vram[CGAData::registerHorizontalSyncWidth] = 10; // 16;
		_vram[CGAData::registerVerticalTotal] = 128 - 1;
		_vram[CGAData::registerVerticalTotalAdjust] = 6;
		_vram[CGAData::registerVerticalDisplayed] = 100;
		_vram[CGAData::registerVerticalSyncPosition] = 112;
		_vram[CGAData::registerInterlaceMode] = 2;
		_vram[CGAData::registerMaximumScanline] = 1;
		_vram[CGAData::registerCursorStart] = 6;
		_vram[CGAData::registerCursorEnd] = 7;
		_vram[CGAData::registerStartAddressHigh] = 0;
		_vram[CGAData::registerStartAddressLow] = 0;
		_vram[CGAData::registerCursorAddressHigh] = 0;
		_vram[CGAData::registerCursorAddressLow] = 0;
		_data.setTotals(238944, 910, 238875);
		_data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);

		_outputSize = _output->requiredSize();

		add(&_bitmap);
		add(&_animated);

		_animated.setDrawWindow(this);
		_animated.setRate(11);

		_xp = 0x8000;
		_yp = 0x8000;
		_a = 0;
		_frame = 0;

		_xPressed = false;
		_zPressed = false;
		_leftPressed = false;
		_rightPressed = false;
		_upPressed = false;
		_downPressed = false;
	}
	~CGAKartWindow() { join(); }
	void join() { _output->join(); }
	void create()
	{
		setText("CGA kart");
		setInnerSize(_outputSize);
		_bitmap.setTopLeft(Vector(0, 0));
		_bitmap.setInnerSize(_outputSize);
		RootWindow::create();
		_animated.start();
	}
	virtual void draw()
	{
		_data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
		_output->restart();
		_animated.restart();

		float s = (400.0/3)*(100 + (100.0/3))/200.0;

		int a = static_cast<int>(_a * 320/tau);
		for (int y = 0; y < 50; ++y) {
			float r = 256*s/(y+1);
			float xx = r*sin(_a);
			float yy = r*cos(_a);

			int xs = static_cast<int>(xx);
			int ys = static_cast<int>(yy);
			int xi = static_cast<int>(yy/40 + 0.5);
			int yi = -static_cast<int>(xx/40 + 0.5);

			xs -= xi*40;
			ys -= yi*40;

			//xs = static_cast<int>(r*sin(_a) - r*cos(_a));
			//ys = static_cast<int>(r*cos(_a) + r*sin(_a));

			//xs = static_cast<int>(r*sin(_a - tau/8)*sqrt(2) + 0.5);
			//ys = static_cast<int>(r*cos(_a - tau/8)*sqrt(2) + 0.5);

			//sin(a + tau/8) = sin(a)*cos(tau/8) + cos(a)*sin(tau/8)
			//cos(a + tau/8) = cos(a)*cos(tau/8) - sin(a)*sin(tau/8)

			//sin(a + tau/8) = (sin(a) + cos(a))/sqrt(2)
			//cos(a + tau/8) = (cos(a) - sin(a))/sqrt(2)

			//sin(a - tau/8)*sqrt(2) = (sin(a) - cos(a))
			//cos(a - tau/8)*sqrt(2) = (cos(a) + sin(a))


			//sin(0) = 0
			//sin(tau/8) = 1/sqrt(2)
			//sin(tau/4) = 1
			//sin(tau/2) = 0
			//sin(3*tau/4) = -1
			//sin(tau) = 0

			//cos(0) = 1
			//cos(tau/8) = 1/sqrt(2)
			//cos(tau/4) = 0
			//cos(tau/2) = -1
			//cos(tau) = 1

			for (int x = 0; x < 80; ++x) {

				_vram[y*80 + x] = skybox[y*320 + (a + x)%320];
				_vram[y*80 + x + 0x2000] = skybox[y*320 + (a + x)%320];

				//float xx = r*sin(_a + x*tau/320);
				//float yy = r*cos(_a + x*tau/320);
				int xw = (xs + _xp) & 0xffff;
				int yw = (ys + _yp) & 0xffff;
				int tile = tileMap[((yw >> 8) & 0xff)*0x100 + ((xw >> 8) & 0xff)];
				int xt = (int)(xw >> 4) & 0x0f;
				int yt = (int)(yw >> 4) & 0x0f;
				int p = tileData[(tile << 8) + (yt << 4) + xt];

				xs += xi;
				ys += yi;

				//int p = ((((int)xw) & 1) == (((int)yw) & 1) ? 0xff : 0x00);
				//p = ((tile & 1) != 0) ? 0xff : 0x00;
				//p = tile;

				//p = tileData[((y & 0xf) << 4) + (x & 0xf)];

				_vram[y*80 + x + 4000] = p;
				_vram[y*80 + x + 4000 + 0x2000] = p;
			}
		}
		//_xp += 1.0/64;
		//_yp += 1.0/64;
		//++_frame;
		//if (_frame == 3) {
		//	_a += tau/320;
		//	_frame = 0;
		//}
		if (_xPressed) {
			if (!_zPressed)
				_angularVelocity += 0.5;
		}
		else {
			if (_zPressed)
				_angularVelocity -= 0.5;
			else
				_angularVelocity *= 0.9;
		}
		if (_angularVelocity < -2)
			_angularVelocity = -2;
		if (_angularVelocity > 2)
			_angularVelocity = 2;
		_a += _angularVelocity*tau/320;
		if (_a < 0)
			_a += tau;
		if (_a >= tau)
			_a -= tau;
		static const int accel = 4;
		if (_upPressed) {
			if (!_downPressed) {
				_xVelocity += accel*sin(_a);
				_yVelocity += accel*cos(_a);
			}
		}
		else {
			if (_downPressed) {
				_xVelocity -= accel*sin(_a);
				_yVelocity -= accel*cos(_a);
			}
		}
		if (_leftPressed) {
			if (!_rightPressed) {
				_xVelocity -= accel*cos(_a);
				_yVelocity += accel*sin(_a);
			}
		}
		else {
			if (_rightPressed) {
				_xVelocity += accel*cos(_a);
				_yVelocity -= accel*sin(_a);
			}
		}
		static const int maxVelocity = 50;
		if (_xVelocity > maxVelocity)
			_xVelocity = maxVelocity;
		if (_xVelocity < -maxVelocity)
			_xVelocity = -maxVelocity;
		if (_yVelocity > maxVelocity)
			_yVelocity = maxVelocity;
		if (_yVelocity < -maxVelocity)
			_yVelocity = -maxVelocity;
		if (!_leftPressed && !_rightPressed && !_upPressed && !_downPressed) {
			_xVelocity *= 0.8;
			_yVelocity *= 0.8;
		}
		_xp += static_cast<int>(_xVelocity);
		_yp += static_cast<int>(_yVelocity);


		//_xp += 12;
		//_yp += 12;
		//_a += tau/320;
		//if (_a > tau)
		//	_a -= tau;
	}
	bool keyboardEvent(int key, bool up)
	{
		switch (key) {
			case VK_RIGHT:
				_rightPressed = !up;
				return true;
			case VK_LEFT:
				_leftPressed = !up;
				return true;
			case VK_UP:
				_upPressed = !up;
				return true;
			case VK_DOWN:
				_downPressed = !up;
				return true;
			case 'Z':
				_zPressed = !up;
				return true;
			case 'X':
				_xPressed = !up;
				return true;
			case VK_ESCAPE:
				PostQuitMessage(0);
				return true;
			//case VK_SPACE:
			//	_spacePressed = !up;
			//	return true;
		}
		return false;
	}
	BitmapWindow* outputWindow() { return &_bitmap; }
	CGAData* getData() { return &_data; }
	CGASequencer* getSequencer() { return &_sequencer; }
private:
	CGAData _data;
	CGASequencer _sequencer;
	CGAOutput* _output;
	ConfigFile* _configFile;
	AnimatedWindow _animated;
	BitmapWindow _bitmap;
	Vector _outputSize;
	Array<Byte> _cgaBytes;
	Byte* _vram;
	int _regs;
	bool _leftPressed;
	bool _rightPressed;
	bool _upPressed;
	bool _downPressed;
	bool _xPressed;
	bool _zPressed;
	float _angularVelocity;
	float _xVelocity;
	float _yVelocity;

	int _frame;
	int _xp;
	int _yp;
	float _a;
};

class Program : public WindowProgram<CGAKartWindow>
{
public:
	void run()
	{
		String chunks =
			File("q:\\projects\\u6\\reem\\chunks", true).contents();
		String map = File("q:\\projects\\u6\\reem\\map", true).contents();
		Bitmap<SRGB> tiles = PNGFileFormat<SRGB>().load(
			File("Q:\\Work\\Website\\Andrew\\computer\\u6maps\\u6tiles.png",
				true));
		String skybox1 = File("q:\\external\\skybox_out.dat", true).contents();
		Bitmap<SRGB> paletteImage = PNGFileFormat<SRGB>().load(
			File("Q:\\8bpp_000_out_cropped.png", true));

		//String chunks =	File("chunks", true).contents();
		//String map = File("map", true).contents();
		//Bitmap<SRGB> tiles = PNGFileFormat<SRGB>().load(
		//	File("u6tiles.png",	true));
		//String skybox1 = File("skybox_out.dat", true).contents();
		//Bitmap<SRGB> paletteImage = PNGFileFormat<SRGB>().load(
		//	File("8bpp_000_out_cropped.png", true));

		skybox.allocate(320*50);
		for (int y = 0; y < 50; ++y)
			for (int x = 0; x < 320; ++x)
				skybox[y*320 + x] = skybox1[y*400 + x + 40];

		tileMap.allocate(0x10000);
		Array<bool> tileUsed(0x100);
		for (int i = 0; i < 0x100; ++i)
			tileUsed[i] = false;
		for (int y = 0; y < 0x100; ++y) {
			for (int x = 0; x < 0x100; ++x) {
				int cx = (x>>3) & 31;
				int cy = (y>>3) & 31;
				int sx = x&7;
				int sy = y&7;
				int a = (((cy<<5) + cx)>>1)*3 + 0x7800;
				int c;
				if ((cx & 1) != 0)
					c = (map[a + 2] << 4) + ((map[a + 1] >> 4) & 15);
				else
					c = ((map[a + 1] & 15) << 8) + map[a];
				int t = chunks[(c<<6) + (sy<<3) + sx];
				tileMap[y*0x100 + x] = t;
				tileUsed[t] = true;
			}
		}
		Array<Byte> tileNewNumber(0x100);
		int nn = 0x10;
		for (int i = 0; i < 0x100; ++i) {
			if (tileUsed[i]) {
				tileNewNumber[i] = nn;
				++nn;
			}
		}


		Array<SRGB> palette(0x100);
		for (int i = 0; i < 0x100; ++i) {
			palette[i] = paletteImage[Vector((i & 1)*40 + 20, (i >> 1)*4 + 1)];
			//printf("%3i %3i %3i\n", palette[i].x, palette[i].y, palette[i].z);
		}

		Array<bool> skipColour(0x100);
		int unskipped = 0;
		for (int i = 0; i < 0x100; ++i) {
			int b0 = ((i & 1) != 0) ? 1 : 0;
			int b1 = ((i & 2) != 0) ? 1 : 0;
			int b2 = ((i & 4) != 0) ? 1 : 0;
			int b3 = ((i & 8) != 0) ? 1 : 0;
			int b4 = ((i & 0x10) != 0) ? 1 : 0;
			int b5 = ((i & 0x20) != 0) ? 1 : 0;
			int b6 = ((i & 0x40) != 0) ? 1 : 0;
			int b7 = ((i & 0x80) != 0) ? 1 : 0;
			int c0 = b0 + b1 + b2 + b3;
			int c1 = b1 + b2 + b3 + b4;
			int c2 = b2 + b3 + b4 + b5;
			int c3 = b3 + b4 + b5 + b6;
			int c4 = b4 + b5 + b6 + b7;
			int c5 = b5 + b6 + b7 + b0;
			int c6 = b6 + b7 + b0 + b1;
			int c7 = b7 + b0 + b1 + b2;
			int ca = c0 - c4;
			int cb = c1 - c5;
			int cc = c2 - c6;
			int cd = c3 - c7;
			skipColour[i] = (ca < -2 || ca > 2 || cb < -2 || cb > 2 || cc < -2 || cc > 2 || cd < -2 || cd > 2);

			//int cl = 0, ch = 0;
			//for (int b = 0; b < 4; ++b) {
			//	if ((i & (1 << b)) != 0)
			//		++cl;
			//	if ((i & (0x10 << b)) != 0)
			//		++ch;
			//}
			//int d = cl - ch;
			//skipColour[i] = (d < -1 || d > 1);
			if (!skipColour[i])
				++unskipped;
		}
		//printf("Unskipped: %i\n", unskipped);

		tileData.allocate(0x10000);
		for (int t = 0; t < 0x100; ++t) {
			for (int y = 0; y < 0x10; ++y) {
				for (int x = 0; x < 0x10; ++x) {
					SRGB c = tiles[Vector((t & 7)*16 + x,(t >> 3)*16 + y)];
					int distance = 0x7fffffff;
					int bestP = 0;
					for (int p = 0; p < 0x100; ++p) {
						//if (skipColour[p])
						//	continue;
						int d = (Vector3Cast<int>(c) - Vector3Cast<int>(palette[p])).modulus2();
						if (d < distance) {
							distance = d;
							bestP = p;
						}
					}
					tileData[(t << 8) + (y << 4) + x] = bestP;
				}
			}
		}
		File("tiles.dat").openWrite().write(tileData);

		//Array<SInt16> vectors(80*50*2);
		//for (int y = 0; y < 50; ++y) {
		//	for (int a = 0; a < 320; ++a) {
		//		float aa = a*tau/320;

		//		float s = (400.0/3)*(100 + (100.0/3))/200.0;
		//		float r = 256*s/(y+1);
		//		float xx = r*sin(aa);
		//		float yy = r*cos(aa);

		//		int xs = static_cast<int>(xx);
		//		int ys = static_cast<int>(yy);
		//		int xi = static_cast<int>(yy/40 + 0.5);
		//		int yi = -static_cast<int>(xx/40 + 0.5);

		//		xs -= xi*40;
		//		ys -= yi*40;

		//		if (a < 80) {
		//			vectors[a*50*2 + y*2] = xs;
		//			vectors[a*50*2 + y*2 + 1] = xi;
		//		}
		//		if (y == 49)
		//			printf("%5i %5i\n", xs, xi);
		//	}
		//}
		//File("vectors.dat").openWrite().write(vectors);


		ConfigFile configFile;
		configFile.addDefaultOption("cgaROM", String("5788005.u33"));
		configFile.addDefaultOption("fftWisdom", String("wisdom"));

		String configName = "default.config";
		if (_arguments.count() >= 2)
			configName = _arguments[1];
		File configPath(configName, true);
		configFile.load(configPath);
		FFTWWisdom<float> wisdom(File(configFile.get<String>("fftWisdom"),
			configPath.parent()));

		CGAOutput output(_window.getData(), _window.getSequencer(),
			_window.outputWindow());
		_window.setOutput(&output);

		_window.setConfig(&configFile, configPath);

		WindowProgram::run();
		_window.join();
	}
};
