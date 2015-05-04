#include <stdlib.h>
#include <string.h>
#include "mupdf/fitz.h"
#include "draw.h"
#include "doc.h"

#define MIN_(a, b)	((a) < (b) ? (a) : (b))

struct doc {
	fz_context *ctx;
	fz_document *pdf;
};

void *doc_draw(struct doc *doc, int p, int zoom, int rotate, int *rows, int *cols)
{
	fz_matrix ctm;		/* transform */
	fz_rect rect;		/* bounds */
	fz_irect bbox;		/* drawing bbox */
	fz_pixmap *pix;
	fz_device *dev;
	fz_page *page;
	fbval_t *pbuf;
	int h, w;
	int x, y;

	if (!(page = fz_load_page(doc->ctx, doc->pdf, p - 1)))
		return NULL;
	fz_rotate(&ctm, rotate);
	fz_pre_scale(&ctm, (float) zoom / 10, (float) zoom / 10);
	fz_bound_page(doc->ctx, page, &rect);
	fz_transform_rect(&rect, &ctm);
	fz_round_rect(&bbox, &rect);
	w = rect.x1 - rect.x0;
	h = rect.y1 - rect.y0;

	pix = fz_new_pixmap_with_bbox(doc->ctx, fz_device_rgb(doc->ctx), &bbox);
	fz_clear_pixmap_with_value(doc->ctx, pix, 0xff);

	dev = fz_new_draw_device(doc->ctx, pix);
	fz_run_page(doc->ctx, page, dev, &ctm, NULL);
	fz_drop_device(doc->ctx, dev);

	if (!(pbuf = malloc(w * h * sizeof(pbuf[0])))) {
		fz_drop_pixmap(doc->ctx, pix);
		fz_drop_page(doc->ctx, page);
		return NULL;
	}
	for (y = 0; y < h; y++) {
		unsigned char *s = fz_pixmap_samples(doc->ctx, pix) +
				y * fz_pixmap_width(doc->ctx, pix) * 4;
		for (x = 0; x < w; x++)
			pbuf[y * w + x] = FB_VAL(s[x * 4 + 0],
					s[x * 4 + 1], s[x * 4 + 2]);
	}
	fz_drop_pixmap(doc->ctx, pix);
	fz_drop_page(doc->ctx, page);
	*cols = w;
	*rows = h;
	return pbuf;
}

int doc_pages(struct doc *doc)
{
	return fz_count_pages(doc->ctx, doc->pdf);
}

struct doc *doc_open(char *path)
{
	struct doc *doc = malloc(sizeof(*doc));
	doc->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	fz_register_document_handlers(doc->ctx);
	fz_try (doc->ctx) {
		doc->pdf = fz_open_document(doc->ctx, path);
	} fz_catch (doc->ctx) {
		fz_drop_context(doc->ctx);
		free(doc);
		return NULL;
	}
	return doc;
}

void doc_close(struct doc *doc)
{
	fz_drop_document(doc->ctx, doc->pdf);
	fz_drop_context(doc->ctx);
	free(doc);
}
