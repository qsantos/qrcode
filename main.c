#include "pbm.h"
#include "qrcode.h"

int main(void)
{
	bitmap_t img;
	pbm_name_to_bitmap(&img, "qr.pbm");

	qrc_decode(&img);

	pbm_del(&img);
	return 0;
}
