import 'dart:io';

import 'package:image/image.dart';
import 'package:path/path.dart' as p;

/// Convert a PDF to a tiled PNG image of all of the pages.
///
/// Requires `pdftoppm` which can be installed on Mac with:
///
///     brew install poppler
Future<void> main(List<String> arguments) async {
  print('Exporting PDF pages to PNG...');
  var tempDir = await Directory('.').createTemp('pages');

  // The `-r` argument is DPI.
  var result = await Process.run('pdftoppm',
      ['-png', '-r', '40', arguments[0], p.join(tempDir.path, 'page')]);
  if (result.exitCode != 0) {
    print('Could not export pages:\n${result.stdout}\n${result.stderr}');
  }

  var pages = <Image>[];
  var imageFiles = tempDir
      .listSync()
      .whereType<File>()
      .where((entry) => entry.path.endsWith('.png'))
      .toList();
  imageFiles.sort((a, b) => a.path.compareTo(b.path));

  for (var imageFile in imageFiles) {
    print('Reading ${imageFile.path}...');
    var bytes = await imageFile.readAsBytes();
    pages.add(decodePng(bytes));
  }

  const columns = 36;
  const rows = 18;
  const border = 4;

  var pageWidth = pages.first.width;
  var pageHeight = pages.first.height;

  var tiled = Image.rgb((pageWidth + border) * columns + border,
      (pageHeight + border) * rows + border);
  tiled.fill(Color.fromRgb(0, 0, 0));

  for (var i = 0; i < pages.length; i++) {
    var x = i % columns;
    var y = i ~/ columns;
    print('Tiling page ${i + 1} ($x, $y)...');
    copyInto(tiled, pages[i],
        dstX: x * (pageWidth + border) + border,
        dstY: y * (pageHeight + border) + border);
  }

  print('Writing pages.png...');
  await File('pages.png').writeAsBytes(encodePng(tiled));

  await tempDir.delete(recursive: true);
}
