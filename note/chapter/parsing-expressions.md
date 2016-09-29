
When talking about error recovery, note that it's less of a big deal today.
Back when machines were super slow, compile runs were infrequent and it was
important to report as many errors correctly as possible in a single run.

Today, compiling is fast and users often only look at the first error in each
compile run, fix it, and retry. Means we don't need very sophisticated error
recovery.

--

Note that we use an EOF token instead of just checking for the end of the token
list because the end of the file may be on a different line than the last
non-whitespace token.

--

When parsing number literals, mention checking for infinity.
