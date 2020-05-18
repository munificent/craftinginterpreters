import 'dart:io';

import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;
import 'package:sass/sass.dart' as sass;

import 'package:tool/src/book.dart';
import 'package:tool/src/markdown.dart';
import 'package:tool/src/text.dart';

// The "(?!-)" is a hack. scanning.md has an inline code sample containing a
// "--" operator. We don't want that to get matched, so fail the match if the
// character after the "-- " is "-", which is the next character in the code
// sample.
final _emDashPattern = RegExp(r"\s+--\s(?!-)");

void main(List<String> arguments) {
  formatFiles();
//  for (var entry in Directory("book").listSync()) {
//    if (entry.path.endsWith(".md")) {
//      print(entry.path);
//    }
//  }

//  buildSass(skipUpToDate: true);

  /*

environment = jinja2.Environment(
    loader=jinja2.FileSystemLoader('asset/template'),
    lstrip_blocks=True,
    trim_blocks=True)

environment.filters['file'] = book.get_file_name

if len(sys.argv) == 2 and sys.argv[1] == "--watch":
  run_server()
  while True:
    format_files(True)
    build_sass(True)
    time.sleep(0.3)
if len(sys.argv) == 2 and sys.argv[1] == "--serve":
  format_files(True)
  run_server()
else:
  format_files(False)
  build_sass(False)

  print("{} words".format(total_words))
   */
}

/// Process each Markdown file.
void formatFiles({bool skipUpToDate = false}) {
  // TODO: Support formatting a single file.
/*
def format_files(skip_up_to_date, one_file=None):
  code_mod = max(
      latest_mod("c/ *.c"),
      latest_mod("c/ *.h"),
      latest_mod("java/com/craftinginterpreters/tool/ *.java"),
      latest_mod("java/com/craftinginterpreters/lox/ *.java"))

  # Reload the source snippets if the code was changed.
  global source_code
  global last_code_load_time
  if not last_code_load_time or code_mod > last_code_load_time:
    source_code = code_snippets.load()
    last_code_load_time = time.time()

  # See if any of the templates were modified. If so, all pages will be rebuilt.
  templates_mod = latest_mod("asset/template/ *.html")
*/

  // TODO: Temp. Just one chapter for now.
  formatFile(Page.all[10]);
  return;

  for (var page in Page.all) {
//    if (one_file == None or page_file == one_file) {
    formatFile(page);
//      format_file(file, skip_up_to_date, max(code_mod, templates_mod))
//    }
  }
}

// TODO: Move to library.
// TODO: Skip up to date stuff.
void formatFile(Page page) {
  print(page.markdownPath);
// def format_file(path, skip_up_to_date, dependencies_mod):
//
//  # See if the HTML is up to date.
//  if skip_up_to_date:
//    source_mod = max(os.path.getmtime(path), dependencies_mod)
//    dest_mod = os.path.getmtime(output_path)
//
//    if source_mod < dest_mod:
//      return
//
//  title = ''
//  title_html = ''
//  part = None
//  template_file = 'page'
//
//  errors = []
  // TODO: Something better typed.
  var sections = <List<Object>>[];
  var headerIndex = 0;
  var subheaderIndex = 0;
  var hasChallenges = false;
  String designNote;
//  snippets = None

  // Read the markdown file and preprocess it.
  var buffer = StringBuffer();

  // Read each line, preprocessing the special codes.
  for (var line in File(page.markdownPath).readAsLinesSync()) {
    var stripped = line.trimLeft();
    var indentation = line.substring(0, line.length - stripped.length);

    if (line.startsWith("^")) {
      print("TODO: Snippet");
      buffer.writeln(line);
//        command,_,arg = stripped.rstrip('\n').lstrip('^').partition(' ')
//        arg = arg.strip()
//
//        if command == 'title':
//          title = arg
//          title_html = title
//
//          # Remove any discretionary hyphens from the title.
//          title = title.replace('&shy;', '')
//
//          # Load the code snippets now that we know the title.
//          snippets = source_code.find_all(title)
//
//          # If there were any errors loading the code, include them.
//          if title in book.CODE_CHAPTERS:
//            errors.extend(source_code.errors[title])
//        elif command == 'part':
//          part = arg
//        elif command == 'template':
//          template_file = arg
//        elif command == 'code':
//          contents = insert_snippet(snippets, arg, contents, errors)
//        else:
//          raise Exception('Unknown command "^{} {}"'.format(command, arg))
//
    } else if (stripped.startsWith("## Challenges")) {
        hasChallenges = true;
        buffer.writeln('<h2><a href="#challenges" name="challenges">'
            'Challenges</a></h2>');
    } else if (stripped.startsWith("## Design Note:")) {
        designNote = stripped.substring('## Design Note:'.length + 1);
        buffer.writeln('<h2><a href="#design-note" name="design-note">'
            'Design Note: $designNote</a></h2>');
    } else if (stripped.startsWith("# ") ||
        stripped.startsWith("## ") ||
        stripped.startsWith("### ")) {
      // Build the section navigation from the headers.
      var index = stripped.indexOf(" ");
      var headerType = stripped.substring(0, index);
      var header = pretty(stripped.substring(index).trim());

      var anchor = toFileName(header);

      // Add an anchor to the header.
      buffer.write("$indentation$headerType ");

      if (headerType.length == 2) {
        headerIndex += 1;
        subheaderIndex = 0;
      } else if (headerType.length == 3) {
        subheaderIndex += 1;
      }

      var number = "${page.number}&#8202;.&#8202;$headerIndex";
      if (headerType.length == 3) number += "&#8202;.&#8202;$subheaderIndex";

      buffer.writeln('<a href="$anchor" name="$anchor">'
          '<small>$number</small> $header</a>\n');

      // Build the section navigation.
      if (headerType.length == 2) {
        sections.add([headerIndex, header]);
      }
    } else {
      buffer.writeln(pretty(line));
    }
  }

//  # Validate that every snippet for the chapter is included.
//  for name, snippet in snippets.items():
//    if name != 'not-yet' and name != 'omit' and snippet != False:
//      errors.append("Unused snippet {}".format(name))
//
//  # Show any errors at the top of the file.
//  if errors:
//    error_markdown = ""
//    for error in errors:
//      error_markdown += "**Error: {}**\n\n".format(error)
//    contents = error_markdown + contents
//

  // TODO: Do this in a cleaner way.
  // Fix up em dashes. We do this on the entire contents instead of in pretty()
  // so that we can handle surrounding whitespace even when the "--" is at the
  // beginning of end of a line in Markdown.
  var contents = buffer
      .toString()
      .replaceAll(_emDashPattern, '<span class="em">&mdash;</span>');

//  # Allow processing markdown inside some tags.
//  contents = contents.replace('<aside', '<aside markdown="1"')
//  contents = contents.replace('<div class="challenges">', '<div class="challenges" markdown="1">')
//  contents = contents.replace('<div class="design-note">', '<div class="design-note" markdown="1">')

  var body = renderMarkdown(contents);

//  # Turn aside markers in code into spans. In the empty span case, insert a
//  # zero-width space because Chrome seems to lose the span's position if it has
//  # no content.
//  # <span class="c1">// [repl]</span>
//  body = ASIDE_COMMENT_PATTERN.sub(r'<span name="\1"> </span>', body)
//  body = ASIDE_WITH_COMMENT_PATTERN.sub(r'<span class="c1" name="\2">// \1</span>', body)
//
//  up = 'Table of Contents'
//  if part:
//    up = part
//  elif title == 'Table of Contents':
//    up = 'Crafting Interpreters'
//
//  data = {
//    'title': title,
//    'part': part,
//    'body': body,
//    'sections': sections,
//    'chapters': get_part_chapters(title),
//    'design_note': design_note,
//    'hasChallenges': hasChallenges,
//    'number': book.chapter_number(title),
//    'prev': book.adjacent_page(title, -1),
//    'prev_type': book.adjacent_type(title, -1),
//    'next': book.adjacent_page(title, 1),
//    'next_type': book.adjacent_type(title, 1),
//    'up': up,
//    'toc': book.TOC
//  }
//
//  template = environment.get_template(template_file + '.html')
//  output = template.render(data)
  // TODO: Use template.
  var output = body;

  // TODO: Temp hack. Insert some whitespace to match the old Markdown.
  output = output.replaceAll("</p></aside>", "</p>\n</aside>");
  output = output.replaceAll("</p><aside", "</p>\n<aside");
  output = output.replaceAll("</div><div", "</div>\n<div");

  // Write the output.
  File(page.htmlPath).writeAsStringSync(output);

//  global total_words
//  global num_chapters
//  global empty_chapters
//
//  word_count = len(contents.split(None))
//  num = book.chapter_number(title)
//  if num:
//    num = '{}. '.format(num)
//
//  # Non-chapter pages aren't counted like regular chapters.
//  if part:
//    num_chapters += 1
//    if word_count < 50:
//      empty_chapters += 1
//      print("    " + term.gray("{}{}".format(num, title)))
//    elif part != "Backmatter" and word_count < 2000:
//      empty_chapters += 1
//      print("  {} {}{} ({} words)".format(
//          term.yellow("-"), num, title, word_count))
//    else:
//      total_words += word_count
//      print("  {} {}{} ({} words)".format(
//          term.green("✓"), num, title, word_count))
//  elif title in ["Crafting Interpreters", "Table of Contents"]:
//    print("{} {}{}".format(term.green("•"), num, title))
//  else:
//    if word_count < 50:
//      print("    " + term.gray("{}{}".format(num, title)))
//    else:
//      print("{} {}{} ({} words)".format(
//          term.green("✓"), num, title, word_count))
}

/// Process each SASS file.
void buildSass({bool skipUpToDate = false}) {
  var modules =
      Glob("asset/sass/*.scss").listSync().map((file) => file.path).toList();

  for (var source in Glob("asset/*.scss").listSync()) {
    var sourcePath = p.normalize(source.path);
    var dest = p.join("site", p.basenameWithoutExtension(source.path) + ".css");

    if (skipUpToDate && !isOutOfDate([...modules, sourcePath], dest)) continue;

    var output =
        sass.compile(sourcePath, color: true, style: sass.OutputStyle.expanded);
    File(dest).writeAsStringSync(output);
    print("$sourcePath -> $dest");
  }
}

/// Returns whether any of the [inputs] have been modified more recently than
/// [output.
bool isOutOfDate(List<String> inputs, String output) {
  var outputModified = File(output).lastModifiedSync();
  for (var input in inputs) {
    var inputModified = File(input).lastModifiedSync();
    if (inputModified.isAfter(outputModified)) return true;
  }

  return false;
}
