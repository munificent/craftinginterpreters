import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

void main(List<String> arguments) {
  if (arguments.isEmpty) {
    print('Usage: benchmark.py [interpreters...] <benchmark>');
    exit(1);
  }

  var interpreters = ['build/clox'];
  var benchmark = arguments.last;
  if (arguments.length > 1) {
    interpreters = arguments.sublist(0, arguments.length - 1);
  }

  if (interpreters.length > 1) {
    runComparison(interpreters, benchmark);
  } else {
    runBenchmark(interpreters[0], benchmark);
  }
}

void runBenchmark(String interpreter, String benchmark) {
  var trial = 1;
  var best = 9999.0;

  for (;;) {
    var elapsed = runTrial(interpreter, benchmark);
    if (elapsed < best) best = elapsed;

    var bestSeconds = best.toStringAsFixed(2);
    print("trial #$trial  $interpreter   best ${bestSeconds}s");
    trial++;
  }
}

/// Runs the benchmark once and returns the elapsed time.
double runTrial(String interpreter, String benchmark) {
  var result = Process.runSync(
      interpreter, [p.join("test", "benchmark", "$benchmark.lox")]);
  var outLines = const LineSplitter().convert(result.stdout as String);

  // Remove the trailing last empty line.
  if (outLines.last == "") outLines.removeLast();

  // The benchmark should print the elapsed time last.
  return double.parse(outLines.last);
}

void runComparison(List<String> interpreters, String benchmark) {
  var trial = 1;
  var best = {for (var interpreter in interpreters) interpreter: 9999.0};

  for (;;) {
    for (var interpreter in interpreters) {
      var elapsed = runTrial(interpreter, benchmark);
      if (elapsed < best[interpreter]) best[interpreter] = elapsed;
    }

    var bestTime = 999.0;
    var worstTime = 0.0;
    String bestInterpreter;
    for (var interpreter in interpreters) {
      if (best[interpreter] < bestTime) {
        bestTime = best[interpreter];
        bestInterpreter = interpreter;
      }
      if (best[interpreter] > worstTime) {
        worstTime = best[interpreter];
      }
    }

    // Turn the time measurement into an effort measurement in units where 1
    // "work" is just the total thing the benchmark does.
    var worstWork = 1.0 / worstTime;

    print("trial #$trial");
    for (var interpreter in interpreters) {
      String suffix;
      if (interpreter == bestInterpreter) {
        var bestWork = 1.0 / best[interpreter];
        var workRatio = bestWork / worstWork;
        var faster = 100 * (workRatio - 1.0);
        suffix = "${faster.toStringAsFixed(4)}% faster";
      } else {
        var ratio = best[interpreter] / bestTime;
        suffix = "${ratio.toStringAsFixed(4)}x time of best";
      }
      var bestString = best[interpreter].toStringAsFixed(4);
      print("  ${interpreter.padRight(30)}   best ${bestString}s  $suffix");
    }

    trial++;
  }
}
