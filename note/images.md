Unscaled source images are scanned at 1200 DPI and stored at around that
resolution. (We'll want the extra pixels when it comes time to do the print
edition.)

Raw scans are saved grayscale unless scanned from graph paper drawings on blue
lines. Those are color, so we can extract the blue channel.

Images are drawn to scale so that text sizes and line thickness is consistent
across illustrations. Each two squares of quarter-ruled graph paper is one
column of the page.

Images are in one of three sizes:

- Normal images are the width of the text column.

- Aside images are the width of the sidebar.
  To avoid being too tall, aside images usually don't fill the entire width of
  the image. Instead, they are scaled to a smaller number of columns, then
  the canvas is grown to pad with pixels.

- Wide images cover the text column and sidebar.

## Web

Web images are scaled to two pixels per CSS pixel (to account for high DPI
screens). They are saved as 64-color PNGs.

The web site is organized into 48 pixel columns. At 2x for high DPI, that's
96 image pixels per column.

Final images are in one of three sizes:

- Normal:              12 columns = 1152 pixels = 24 graph paper squares
- Aside:                6 columns =  576 pixels
- Wide:   12 + 6 + 1 = 19 columns = 1824 pixels

## Print

Print images are 1200 DPI bitmaps saved as TIFF files. Sizes:

- Normal: 324 pts = 4.5  inches = 5400 pixels
- Aside:  126 pts = 1.75 inches = 2100 pixels
- Wide:   468 pts = 6.5  inches = 7800 pixels
