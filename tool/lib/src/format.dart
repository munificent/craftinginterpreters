/// The book format being rendered to.
enum Format {
  /// HTML for the web.
  web,

  /// XML for importing into InDesign.
  print,
}

extension FormatExtension on Format {
  bool get isWeb => this == Format.web;
  bool get isPrint => this == Format.print;
}
