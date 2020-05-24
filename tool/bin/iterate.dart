import 'dart:io';

import 'package:path/path.dart' as p;

import 'build.dart' as build;

void main(List<String> arguments) {
  if (arguments.length != 1) {
    print("Options:");
    print("iterate.dart old # Copy old files to working directory.");
    print("iterate.dart new # Copy new files to working directory.");
    print(
        "iterate.dart run # Generate and copy new files to working directory.");
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
      source = source.replaceAll('class="nn"', 'class="n"');
      source = source.replaceAll('class="nf"', 'class="n"');
      source = source.replaceAll('class="nx"', 'class="n"');

      source = source.replaceAll('class="vg"', 'class="nc"');

      source = source.replaceAll('class="sc"', 'class="s"');
      source = source.replaceAll('class="s1"', 'class="s"');
      source = source.replaceAll('class="s2"', 'class="s"');

      // TODO: We don't color this at all, so we could remove the class
      // entirely.
      source = source.replaceAll('class="p"', 'class="o"');

      // For some reason, Pygments puts empty spans at the end of preprocessor
      // lines.
      source = source.replaceAll('<span class="cp"></span>', "");

      source = source.replaceAll('<aside markdown="1"', "<aside");

      // The old files don't have a newline at the end.
      if (source.endsWith("</html>")) source += "\n";

      // Sometimes Pygments doesn't collapse adjacent operator spans.
      for (var i = 0; i < 5; i++) {
        source = source.replaceAllMapped(
            RegExp(r'<span class="o">([^<]+)</span>'
                r'<span class="o">([^<]+)</span>'),
            (match) => '<span class="o">${match[1]}${match[2]}</span>');
      }

      // Python Markdown preserves leading indentation for bullet list items.
      // Super hacky fix to strip out that leading indentation but not from
      // other lines.
      source = source.replaceAllMapped(RegExp(r"\n +(\w.*)"), (match) {
        var text = match[1];
        const nonListLines = [
          "ga('",
          "s.getElements",
          "-->",
          "Next Chapter: ",
          "Part: &ldquo;",
          "2 39 ",
          "15 | function"
        ];
        if (nonListLines.any(text.contains)) return match[0];

        return "\n$text";
      });

      source = source.replaceAll('\n</a></h2>\n\n', '</a></h2>\n');

      File(outPath).writeAsStringSync(source);
      print(outPath);
    }
  }
}
