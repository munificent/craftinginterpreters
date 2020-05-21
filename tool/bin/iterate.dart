import 'dart:io';

import 'package:path/path.dart' as p;

import 'build.dart' as build;

void main(List<String> arguments) {
  if (arguments.length != 1) {
    print("Options:");
    print("iterate.dart old # Copy old files to working directory.");
    print("iterate.dart new # Copy new files to working directory.");
    print("iterate.dart run # Generate and copy new files to working directory.");
    exit(1);
  }

  switch (arguments[0]) {
    case "old":
      copyDirectory("site");
      break;
    case "new":
      copyDirectory("build/site_dart");
      break;
    case "run":
      build.main([]);
      copyDirectory("build/site_dart");
      break;
  }
}

void copyDirectory(String from) {
  // Ensure the output directory exists.
  Directory(p.join("build", "working")).createSync(recursive: true);

  var files = Directory(from)
      .listSync()
      .where((file) => file.path.endsWith(".html"))
      .map((file) => file.path)
      .toList();
  files.sort();

  for (var file in files) {
    if (file.endsWith(".html")) {
      var outPath = p.join("build", "working", p.basename(file));

      var source = File(file).readAsStringSync();

      // Normalize CSS syntax highlight classes that are visually identical.
      source = source.replaceAll('class="kd"', 'class="k"');
      source = source.replaceAll('class="kt"', 'class="k"');
      source = source.replaceAll('class="kn"', 'class="k"');
      source = source.replaceAll('class="kr"', 'class="k"');

      source = source.replaceAll('class="na"', 'class="n"');
      source = source.replaceAll('class="nc"', 'class="n"');
      source = source.replaceAll('class="nn"', 'class="n"');
      source = source.replaceAll('class="nf"', 'class="n"');

      source = source.replaceAll('class="vg"', 'class="nc"');

      source = source.replaceAll('class="sc"', 'class="s"');
      source = source.replaceAll('class="s1"', 'class="s"');
//      source = source.replaceAll('class="cpf"', 'class="s"');

      // TODO: We don't color this at all, so we could remove the class
      // entirely.
      source = source.replaceAll('class="p"', 'class="o"');

      // For some reason, Pygments puts empty spans at the end of preprocessor
      // lines.
      source = source.replaceAll('<span class="cp"></span>', "");

      // Sometimes Pygments doesn't collapse adjacent operator spans.
      source = source.replaceAllMapped(
          RegExp(r'<span class="o">([^<]+)</span>'
              r'<span class="o">([^<]+)</span>'),
          (match) => '<span class="o">${match[1]}${match[2]}</span>');

      File(outPath).writeAsStringSync(source);
      print(outPath);
    }
  }
}
