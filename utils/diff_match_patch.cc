/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: fraser@google.com (Neil Fraser)
 * Author: mikeslemmer@gmail.com (Mike Slemmer)
 * Author: quentinfiard@gmail.com (Quentin Fiard)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Diff Match and Patch
 * http://code.google.com/p/google-diff-match-patch/
 */

// this line need comment for Linux 
#include "pgAdmin3.h"
#ifndef __WXMSW__
#include "utils/diff_match_patch.h"
#endif



typedef std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>
    UnicodeEncoder;

//////////////////////////
//
// Diff Class
//
//////////////////////////

/**
 * Constructor.  Initializes the diff with the provided values.
 * @param operation One of INSERT, DELETE or EQUAL
 * @param text The text being applied
 */
Diff::Diff(Operation _operation, const std::wstring &_text)
    : operation(_operation), text(_text) {
  // Construct a diff with the specified operation and text.
}

Diff::Diff() {}

std::wstring Diff::strOperation(Operation op) {
  switch (op) {
    case INSERT:
      return L"INSERT";
    case DELETE:
      return L"DELETE";
    case EQUAL:
      return L"EQUAL";
  }
  throw "Invalid operation.";
}

/**
 * Display a human-readable version of this Diff.
 * @return text version
 */
std::wstring Diff::toString() const {
  std::wstring prettyText = text;
  // Replace linebreaks with Pilcrow signs.
  std::replace(prettyText.begin(), prettyText.end(), L'\n', L'\u00b6');
  return L"Diff(" + strOperation(operation) + L",\"" + prettyText + L"\")";
}

/**
 * Is this Diff equivalent to another Diff?
 * @param d Another Diff to compare against
 * @return true or false
 */
bool Diff::operator==(const Diff &d) const {
  return (d.operation == this->operation) && (d.text == this->text);
}

bool Diff::operator!=(const Diff &d) const { return !(operator==(d)); }

/////////////////////////////////////////////
//
// Patch Class
//
/////////////////////////////////////////////

namespace {

int64_t AsInt64(const std::wstring &str) {
  int64_t res;
  if (swscanf(str.c_str(), L"%lld", &res) != 1) {
    UnicodeEncoder unicode_encoder;
    throw "Not an int64: " + unicode_encoder.to_bytes(str);
  }
  return res;
}

std::size_t AsSizeT(const std::wstring &str) {
  std::size_t res;
  if (swscanf(str.c_str(), L"%lu", &res) != 1) {
    UnicodeEncoder unicode_encoder;
    throw "Not a size_t: " + unicode_encoder.to_bytes(str);
  }
  return res;
}

std::string AsString(std::size_t value) {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

bool EndsWith(const std::wstring &str, const std::wstring &suffix) {
  return str.size() >= suffix.size() &&
         str.substr(str.size() - suffix.size()) == suffix;
}

// Replaces all occurences of character c in string str with the given
// replacement string.
void ReplaceAll(std::wstring &str, wchar_t c, const std::wstring &replacement) {
  std::size_t pos = 0;
  while ((pos = str.find(c, pos)) != std::wstring::npos) {
    str.replace(pos, 1, replacement);
    pos += replacement.size();
  }
}

template <class Container>
void Split(const std::wstring &str, wchar_t sep, Container &output) {
  output.clear();
  std::size_t last_pos = 0;
  std::size_t pos = 0;
  while ((pos = str.find(sep, pos)) != std::wstring::npos) {
    output.push_back(str.substr(last_pos, pos - last_pos));
    pos += 1;
    last_pos = pos;
  }
  output.push_back(str.substr(last_pos));
}

bool StartsWith(const std::wstring &str, const std::wstring &suffix) {
  return str.size() >= suffix.size() && str.substr(0, suffix.size()) == suffix;
}

std::wstring URLDecode(const std::wstring &text) {
  UnicodeEncoder unicode_encoder;
  const auto &unicode = unicode_encoder.to_bytes(text);
  std::stringstream res;
  auto it = unicode.begin();
  auto current = it;
  auto next = (it != unicode.end()) ? ++it : unicode.end();
  auto nextnext = (it != unicode.end()) ? ++it : unicode.end();
  for (; nextnext != unicode.end();) {
    if ((*current == '%') && (isxdigit(*next) && isxdigit(*nextnext))) {
      auto end = nextnext;
      std::string hex(next, ++end);
      uint32_t c;
      sscanf(hex.c_str(), "%x", &c);
      res << (char)c;
      current = nextnext;
      next = ++current;
      if (next != unicode.end()) ++next;
      nextnext = next;
      if (nextnext != unicode.end()) ++nextnext;
    } else {
      res << *current;
      ++current, ++next, ++nextnext;
    }
  }
  if (current != unicode.end()) res << *current;
  if (next != unicode.end()) res << *next;
  return unicode_encoder.from_bytes(res.str());
}

void URLEncodeCharacter(char c, std::stringstream &out) {
  if (!iswalnum(c) && c != '-' && c != '.' && c != '_' && c != '~') {
    out << '%' << std::setw(2) << std::setfill('0') << std::hex
        << std::uppercase << (int)(static_cast<unsigned char>(c));
  } else {
    out << c;
  }
}

std::wstring URLEncode(const std::wstring &text, const std::wstring &exclude) {
  std::unordered_set<wchar_t> exclude_set(exclude.begin(), exclude.end());
  std::stringstream res;
  UnicodeEncoder unicode_encoder;
  for (auto c : text) {
    const auto &unicode = unicode_encoder.to_bytes(std::wstring(1, c));
    if (exclude_set.find(c) == exclude_set.end() && !iswalnum(c) && c != L'-' &&
        c != L'.' && c != L'_' && c != L'~') {
      for (const auto narrow_c : unicode) {
        URLEncodeCharacter(narrow_c, res);
      }
    } else {
      res << unicode;
    }
  }
  return unicode_encoder.from_bytes(res.str());
}

}  // namespace

/**
 * Constructor.  Initializes with an empty list of diffs.
 */
Patch::Patch() : start1(0), start2(0), size1(0), size2(0) {}

bool Patch::isNull() const {
  if (start1 == 0 && start2 == 0 && size1 == 0 && size2 == 0 &&
      diffs.size() == 0) {
    return true;
  }
  return false;
}

/**
 * Emmulate GNU diff's format.
 * Header: @@ -382,8 +481,9 @@
 * Indicies are printed as 1-based, not 0-based.
 * @return The GNU diff string
 */
std::wstring Patch::toString() const {
  std::wstringstream coords1, coords2;
  if (size1 == 0) {
    coords1 << start1 << L",0";
  } else if (size1 == 1) {
    coords1 << start1 + 1;
  } else {
    coords1 << start1 + 1 << L',' << size1;
  }
  if (size2 == 0) {
    coords2 << start2 << L",0";
  } else if (size2 == 1) {
    coords2 << start2 + 1;
  } else {
    coords2 << start2 + 1 << L',' << size2;
  }
  std::wstringstream text;
  text << L"@@ -" << coords1.str() << L" +" << coords2.str() << L" @@\n";
  // Escape the body of the patch with %xx notation.
  for (const auto &aDiff : diffs) {
    switch (aDiff.operation) {
      case INSERT:
        text << L'+';
        break;
      case DELETE:
        text << L'-';
        break;
      case EQUAL:
        text << L' ';
        break;
    }
    text << URLEncode(aDiff.text, L" !~*'();/?:@&=+$,#") + L'\n';
  }

  return text.str();
}

/////////////////////////////////////////////
//
// diff_match_patch Class
//
/////////////////////////////////////////////

diff_match_patch::diff_match_patch(short Diff_EditCost_, float Match_Threshold_,int Match_Distance_)
    : Diff_Timeout(4.0f),
      Diff_EditCost(Diff_EditCost_), //4
      Match_Threshold(Match_Threshold_), //0.5f
      Match_Distance(Match_Distance_),  //1000
      Patch_DeleteThreshold(0.5f),
      Patch_Margin(4),
      Match_MaxBits(32) {}

std::list<Diff> diff_match_patch::diff_main(const std::wstring &text1,
                                            const std::wstring &text2) {
  return diff_main(text1, text2, true);
}

std::list<Diff> diff_match_patch::diff_main(const std::wstring &text1,
                                            const std::wstring &text2,
                                            bool checklines) {
  // Set a deadline by which time the diff must be complete.
  clock_t deadline;
  if (Diff_Timeout <= 0) {
    deadline = std::numeric_limits<clock_t>::max();
  } else {
    deadline = clock() + (clock_t)(Diff_Timeout * CLOCKS_PER_SEC);
  }
  return diff_main(text1, text2, checklines, deadline);
}

std::list<Diff> diff_match_patch::diff_main(const std::wstring &text1,
                                            const std::wstring &text2,
                                            bool checklines, clock_t deadline) {
  // Check for equality (speedup).
  std::list<Diff> diffs;
  if (text1 == text2) {
    if (!text1.empty()) {
      diffs.push_back(Diff(EQUAL, text1));
    }
    return diffs;
  }

  // Trim off common prefix (speedup).
  std::size_t commonsize = diff_commonPrefix(text1, text2);
  const std::wstring &commonprefix = text1.substr(0, commonsize);
  std::wstring textChopped1 = text1.substr(commonsize);
  std::wstring textChopped2 = text2.substr(commonsize);

  // Trim off common suffix (speedup).
  commonsize = diff_commonSuffix(textChopped1, textChopped2);
  const std::wstring &commonsuffix =
      textChopped1.substr(textChopped1.size() - commonsize);
  textChopped1 = textChopped1.substr(0, textChopped1.size() - commonsize);
  textChopped2 = textChopped2.substr(0, textChopped2.size() - commonsize);

  // Compute the diff on the middle block.
  diffs = diff_compute(textChopped1, textChopped2, checklines, deadline);

  // Restore the prefix and suffix.
  if (!commonprefix.empty()) {
    diffs.push_front(Diff(EQUAL, commonprefix));
  }
  if (!commonsuffix.empty()) {
    diffs.push_back(Diff(EQUAL, commonsuffix));
  }

  diff_cleanupMerge(diffs);

  return diffs;
}

std::list<Diff> diff_match_patch::diff_compute(std::wstring text1,
                                               std::wstring text2,
                                               bool checklines,
                                               clock_t deadline) {
  std::list<Diff> diffs;

  if (text1.empty()) {
    // Just add some text (speedup).
    diffs.push_back(Diff(INSERT, text2));
    return diffs;
  }

  if (text2.empty()) {
    // Just delete some text (speedup).
    diffs.push_back(Diff(DELETE, text1));
    return diffs;
  }

  {
    const std::wstring longtext = text1.size() > text2.size() ? text1 : text2;
    const std::wstring shorttext = text1.size() > text2.size() ? text2 : text1;
    const std::size_t i = longtext.find(shorttext);
    if (i != std::wstring::npos) {
      // Shorter text is inside the longer text (speedup).
      const Operation op = (text1.size() > text2.size()) ? DELETE : INSERT;
      diffs.push_back(Diff(op, longtext.substr(0, i)));
      diffs.push_back(Diff(EQUAL, shorttext));
      diffs.push_back(Diff(op, safeSubStr(longtext, i + shorttext.size())));
      return diffs;
    }

    if (shorttext.size() == 1) {
      // Single character string.
      // After the previous speedup, the character can't be an equality.
      diffs.push_back(Diff(DELETE, text1));
      diffs.push_back(Diff(INSERT, text2));
      return diffs;
    }
    // Garbage collect longtext and shorttext by scoping out.
  }

  // Check to see if the problem can be split in two.
  const std::vector<std::wstring> hm = diff_halfMatch(text1, text2);
  if (hm.size() > 0) {
    // A half-match was found, sort out the return data.
    const std::wstring text1_a = hm[0];
    const std::wstring text1_b = hm[1];
    const std::wstring text2_a = hm[2];
    const std::wstring text2_b = hm[3];
    const std::wstring mid_common = hm[4];
    // Send both pairs off for separate processing.
    std::list<Diff> diffs_a = diff_main(text1_a, text2_a, checklines, deadline);
    std::list<Diff> diffs_b = diff_main(text1_b, text2_b, checklines, deadline);
    // Merge the results.
    diffs.splice(diffs.end(), diffs_a);
    diffs.push_back(Diff(EQUAL, mid_common));
    diffs.splice(diffs.end(), diffs_b);
    return diffs;
  }

  // Perform a real diff.
  if (checklines && text1.size() > 100 && text2.size() > 100) {
    return diff_lineMode(text1, text2, deadline);
  }

  return diff_bisect(text1, text2, deadline);
}

std::list<Diff> diff_match_patch::diff_lineMode(std::wstring text1,
                                                std::wstring text2,
                                                clock_t deadline) {
  // Scan the text on a line-by-line basis first.
  const auto &b = diff_linesToChars(text1, text2);
  text1 = std::get<0>(b);
  text2 = std::get<1>(b);
  const auto &line_array = std::get<2>(b);

  std::list<Diff> diffs = diff_main(text1, text2, false, deadline);

  // Convert the diff back to original text.
  diff_charsToLines(diffs, line_array);
  // Eliminate freak matches (e.g. blank lines)
  diff_cleanupSemantic(diffs);

  // Rediff any replacement blocks, this time character-by-character.
  // Add a dummy entry at the end.
  diffs.push_back(Diff(EQUAL, L""));
  std::size_t count_delete = 0;
  std::size_t count_insert = 0;
  std::wstring text_delete = L"";
  std::wstring text_insert = L"";

  for (auto thisDiff = diffs.begin(); thisDiff != diffs.end();) {
    switch (thisDiff->operation) {
      case INSERT:
        count_insert++;
        text_insert += thisDiff->text;
        ++thisDiff;
        break;
      case DELETE:
        count_delete++;
        text_delete += thisDiff->text;
        ++thisDiff;
        break;
      case EQUAL:
        // Upon reaching an equality, check for prior redundancies.
        if (count_delete >= 1 && count_insert >= 1) {
          // Delete the offending records and add the merged ones.
          auto it = thisDiff;
          for (std::size_t j = 0; j < count_delete + count_insert; j++) {
            --it;
            it = diffs.erase(it);
          }
          auto new_diffs = diff_main(text_delete, text_insert, false, deadline);
          for (const auto &new_diff : new_diffs) {
            diffs.insert(it, new_diff);
          }
        } else {
          ++thisDiff;
        }
        count_insert = 0;
        count_delete = 0;
        text_delete = L"";
        text_insert = L"";
        break;
    }
  }
  diffs.pop_back();  // Remove the dummy entry at the end.

  return diffs;
}

std::list<Diff> diff_match_patch::diff_bisect(const std::wstring &text1,
                                              const std::wstring &text2,
                                              clock_t deadline) {
  // Cache the text sizes to prevent multiple calls.
  const int64_t text1_size = text1.size();
  const int64_t text2_size = text2.size();
  const int64_t max_d = (text1_size + text2_size + 1) / 2;
  const int64_t v_offset = max_d;
  const int64_t v_size = 2 * max_d;
  std::unique_ptr<int64_t[]> v1(new int64_t[v_size]);
  std::unique_ptr<int64_t[]> v2(new int64_t[v_size]);
  for (std::size_t x = 0; x < v_size; x++) {
    v1[x] = -1;
    v2[x] = -1;
  }
  v1[v_offset + 1] = 0;
  v2[v_offset + 1] = 0;
  const int64_t delta = text1_size - text2_size;
  // If the total number of characters is odd, then the front path will
  // collide with the reverse path.
  const bool front = (delta % 2 != 0);
  // Offsets for start and end of k loop.
  // Prevents mapping of space beyond the grid.
  int64_t k1start = 0;
  int64_t k1end = 0;
  int64_t k2start = 0;
  int64_t k2end = 0;
  for (int64_t d = 0; d < max_d; d++) {
    // Bail out if deadline is reached.
    if (clock() > deadline) {
      break;
    }

    // Walk the front path one step.
    for (int64_t k1 = -d + k1start; k1 <= d - k1end; k1 += 2) {
      const int64_t k1_offset = v_offset + k1;
      int64_t x1;
      if (k1 == -d || (k1 != d && v1[k1_offset - 1] < v1[k1_offset + 1])) {
        x1 = v1[k1_offset + 1];
      } else {
        x1 = v1[k1_offset - 1] + 1;
      }
      int64_t y1 = x1 - k1;
      while (x1 < text1_size && y1 < text2_size && text1[x1] == text2[y1]) {
        x1++;
        y1++;
      }
      v1[k1_offset] = x1;
      if (x1 > text1_size) {
        // Ran off the right of the graph.
        k1end += 2;
      } else if (y1 > text2_size) {
        // Ran off the bottom of the graph.
        k1start += 2;
      } else if (front) {
        int64_t k2_offset = v_offset + delta - k1;
        if (k2_offset >= 0 && k2_offset < v_size && v2[k2_offset] != -1) {
          // Mirror x2 onto top-left coordinate system.
          int64_t x2 = text1_size - v2[k2_offset];
          if (x1 >= x2) {
            // Overlap detected.
            return diff_bisectSplit(text1, text2, x1, y1, deadline);
          }
        }
      }
    }

    // Walk the reverse path one step.
    for (int64_t k2 = -d + k2start; k2 <= d - k2end; k2 += 2) {
      const int64_t k2_offset = v_offset + k2;
      int64_t x2;
      if (k2 == -d || (k2 != d && v2[k2_offset - 1] < v2[k2_offset + 1])) {
        x2 = v2[k2_offset + 1];
      } else {
        x2 = v2[k2_offset - 1] + 1;
      }
      int64_t y2 = x2 - k2;
      while (x2 < text1_size && y2 < text2_size &&
             text1[text1_size - x2 - 1] == text2[text2_size - y2 - 1]) {
        x2++;
        y2++;
      }
      v2[k2_offset] = x2;
      if (x2 > text1_size) {
        // Ran off the left of the graph.
        k2end += 2;
      } else if (y2 > text2_size) {
        // Ran off the top of the graph.
        k2start += 2;
      } else if (!front) {
        int64_t k1_offset = v_offset + delta - k2;
        if (k1_offset >= 0 && k1_offset < v_size && v1[k1_offset] != -1) {
          int64_t x1 = v1[k1_offset];
          int64_t y1 = v_offset + x1 - k1_offset;
          // Mirror x2 onto top-left coordinate system.
          x2 = text1_size - x2;
          if (x1 >= x2) {
            // Overlap detected.
            return diff_bisectSplit(text1, text2, x1, y1, deadline);
          }
        }
      }
    }
  }
  // Diff took too long and hit the deadline or
  // number of diffs equals number of characters, no commonality at all.
  std::list<Diff> diffs;
  diffs.push_back(Diff(DELETE, text1));
  diffs.push_back(Diff(INSERT, text2));
  return diffs;
}

std::list<Diff> diff_match_patch::diff_bisectSplit(const std::wstring &text1,
                                                   const std::wstring &text2,
                                                   std::size_t x, std::size_t y,
                                                   clock_t deadline) {
  std::wstring text1a = text1.substr(0, x);
  std::wstring text2a = text2.substr(0, y);
  std::wstring text1b = safeSubStr(text1, x);
  std::wstring text2b = safeSubStr(text2, y);

  // Compute both diffs serially.
  std::list<Diff> diffs = diff_main(text1a, text2a, false, deadline);
  std::list<Diff> diffsb = diff_main(text1b, text2b, false, deadline);
  diffs.splice(diffs.end(), diffsb);
  return diffs;
}

std::tuple<std::wstring, std::wstring, std::vector<std::wstring> >
diff_match_patch::diff_linesToChars(const std::wstring &text1,
                                    const std::wstring &text2) const {
  std::vector<std::wstring> line_array;
  std::unordered_map<std::wstring, std::size_t> lineHash;
  // e.g. line_array[4] == "Hello\n"
  // e.g. linehash.get("Hello\n") == 4

  // "\x00" is a valid character, but various debuggers don't like it.
  // So we'll insert a junk entry to avoid generating a null character.
  line_array.push_back(L"");

  const std::wstring chars1 =
      diff_linesToCharsMunge(text1, line_array, lineHash);
  const std::wstring chars2 =
      diff_linesToCharsMunge(text2, line_array, lineHash);

  return std::make_tuple(chars1, chars2, line_array);
}

std::wstring diff_match_patch::diff_linesToCharsMunge(
    const std::wstring &text, std::vector<std::wstring> &line_array,
    std::unordered_map<std::wstring, std::size_t> &lineHash) const {
  std::size_t lineStart = 0;
  bool has_line_end = false;
  std::size_t lineEnd = std::wstring::npos;
  std::wstring line;
  std::wstring chars;

  if (text.size() == 0) return chars;

  // Walk the text, pulling out a substring for each line.
  // text.split('\n') would would temporarily double our memory footprint.
  // Modifying text would create many large strings to garbage collect.
  while (!has_line_end || lineEnd < text.size() - 1) {
    lineEnd = text.find('\n', lineStart);
    has_line_end = true;
    if (lineEnd == std::wstring::npos) {
      lineEnd = text.size() - 1;
    }
    line = safeSubStr(text, lineStart, lineEnd + 1 - lineStart);
    lineStart = lineEnd + 1;

    if (lineHash.find(line) != lineHash.end()) {
      chars += wchar_t(static_cast<ushort>(lineHash[line]));
    } else {
      line_array.push_back(line);
      lineHash.emplace(line, line_array.size() - 1);
      chars += wchar_t(static_cast<ushort>(line_array.size() - 1));
    }
  }
  return chars;
}

void diff_match_patch::diff_charsToLines(
    std::list<Diff> &diffs, const std::vector<std::wstring> &line_array) {
  for (auto &diff : diffs) {
    std::wstring text;
    for (std::size_t y = 0; y < diff.text.size(); y++) {
      text += line_array[static_cast<ushort>(diff.text[y])];
    }
    diff.text = text;
  }
}

std::size_t diff_match_patch::diff_commonPrefix(const std::wstring &text1,
                                                const std::wstring &text2) {
  // Performance analysis: http://neil.fraser.name/news/2007/10/09/
  const std::size_t n = std::min(text1.size(), text2.size());
  for (std::size_t i = 0; i < n; i++) {
    if (text1[i] != text2[i]) {
      return i;
    }
  }
  return n;
}

std::size_t diff_match_patch::diff_commonSuffix(const std::wstring &text1,
                                                const std::wstring &text2) {
  // Performance analysis: http://neil.fraser.name/news/2007/10/09/
  const std::size_t text1_size = text1.size();
  const std::size_t text2_size = text2.size();
  const std::size_t n = std::min(text1_size, text2_size);
  for (std::size_t i = 1; i <= n; i++) {
    if (text1[text1_size - i] != text2[text2_size - i]) {
      return i - 1;
    }
  }
  return n;
}

std::size_t diff_match_patch::diff_commonOverlap(const std::wstring &text1,
                                                 const std::wstring &text2) {
  // Cache the text sizes to prevent multiple calls.
  const std::size_t text1_size = text1.size();
  const std::size_t text2_size = text2.size();
  // Eliminate the null case.
  if (text1_size == 0 || text2_size == 0) {
    return 0;
  }
  // Truncate the longer string.
  std::wstring text1_trunc = text1;
  std::wstring text2_trunc = text2;
  if (text1_size > text2_size) {
    text1_trunc = text1.substr(text1.size() - text2_size);
  } else if (text1_size < text2_size) {
    text2_trunc = text2.substr(0, text1_size);
  }
  const std::size_t text_size = std::min(text1_size, text2_size);
  // Quick check for the worst case.
  if (text1_trunc == text2_trunc) {
    return text_size;
  }

  // Start by looking for a single character match
  // and increase size until no match is found.
  // Performance analysis: http://neil.fraser.name/news/2010/11/04/
  std::size_t best = 0;
  std::size_t size = 1;
  while (true) {
    std::wstring pattern = text1_trunc.substr(text1_trunc.size() - size);
    std::size_t found = text2_trunc.find(pattern);
    if (found == std::wstring::npos) {
      return best;
    }
    size += found;
    if (found == 0 || text1_trunc.substr(text1_trunc.size() - size) ==
                          text2_trunc.substr(0, size)) {
      best = size;
      size++;
    }
  }
}

std::vector<std::wstring> diff_match_patch::diff_halfMatch(
    const std::wstring &text1, const std::wstring &text2) {
  if (Diff_Timeout <= 0) {
    // Don't risk returning a non-optimal diff if we have unlimited time.
    return std::vector<std::wstring>();
  }
  const std::wstring longtext = text1.size() > text2.size() ? text1 : text2;
  const std::wstring shorttext = text1.size() > text2.size() ? text2 : text1;
  if (longtext.size() < 4 || shorttext.size() * 2 < longtext.size()) {
    return std::vector<std::wstring>();  // Pointless.
  }

  // First check if the second quarter is the seed for a half-match.
  const std::vector<std::wstring> hm1 =
      diff_halfMatchI(longtext, shorttext, (longtext.size() + 3) / 4);
  // Check again based on the third quarter.
  const std::vector<std::wstring> hm2 =
      diff_halfMatchI(longtext, shorttext, (longtext.size() + 1) / 2);
  std::vector<std::wstring> hm;
  if (hm1.empty() && hm2.empty()) {
    return std::vector<std::wstring>();
  } else if (hm2.empty()) {
    hm = hm1;
  } else if (hm1.empty()) {
    hm = hm2;
  } else {
    // Both matched.  Select the longest.
    hm = hm1[4].size() > hm2[4].size() ? hm1 : hm2;
  }

  // A half-match was found, sort out the return data.
  if (text1.size() > text2.size()) {
    return hm;
  } else {
	  std::vector<std::wstring> listRet;
	  listRet.push_back(hm[2]);
	  listRet.push_back(hm[3]);
	  listRet.push_back(hm[0]);
	  listRet.push_back(hm[1]);
	  listRet.push_back(hm[4]);
    return listRet;
  }
}

std::vector<std::wstring> diff_match_patch::diff_halfMatchI(
    const std::wstring &longtext, const std::wstring &shorttext,
    std::size_t i) {
  // Start with a 1/4 size substring at position i as a seed.
  const std::wstring seed = safeSubStr(longtext, i, longtext.size() / 4);
  std::size_t j=0;
  std::wstring best_common;
  std::wstring best_longtext_a, best_longtext_b;
  std::wstring best_shorttext_a, best_shorttext_b;
  while ((j = shorttext.find(seed, j + 1)) != std::wstring::npos) {
    const std::size_t prefixLength =
        diff_commonPrefix(safeSubStr(longtext, i), safeSubStr(shorttext, j));
    const std::size_t suffixLength =
        diff_commonSuffix(longtext.substr(0, i), shorttext.substr(0, j));
    if (best_common.size() < suffixLength + prefixLength) {
      best_common = safeSubStr(shorttext, j - suffixLength, suffixLength) +
                    safeSubStr(shorttext, j, prefixLength);
      best_longtext_a = longtext.substr(0, i - suffixLength);
      best_longtext_b = safeSubStr(longtext, i + prefixLength);
      best_shorttext_a = shorttext.substr(0, j - suffixLength);
      best_shorttext_b = safeSubStr(shorttext, j + prefixLength);
    }
  }
  if (best_common.size() * 2 >= longtext.size()) {
	 std::vector<std::wstring> aa;
	 aa.push_back(best_longtext_a);
	 aa.push_back(best_longtext_b);
	 aa.push_back(best_shorttext_a);
	 aa.push_back(best_shorttext_b);
	 aa.push_back(best_common);
     return aa;
  } else {
    return std::vector<std::wstring>();
  }
}

void diff_match_patch::diff_cleanupSemantic(std::list<Diff> &diffs) {
  if (diffs.empty()) {
    return;
  }
  bool changes = false;
  std::vector<std::list<Diff>::iterator> equalities;  // Stack of equalities.
  bool has_last_equality = false;
  std::wstring last_equality;  // Always equal to equalities.back().text
  // Number of characters that changed prior to the equality.
  std::size_t size_insertions1 = 0;
  std::size_t size_deletions1 = 0;
  // Number of characters that changed after the equality.
  std::size_t size_insertions2 = 0;
  std::size_t size_deletions2 = 0;

  for (auto thisDiff = diffs.begin(); thisDiff != diffs.end();) {
    if (thisDiff->operation == EQUAL) {
      // Equality found.
      equalities.push_back(thisDiff);
      size_insertions1 = size_insertions2;
      size_deletions1 = size_deletions2;
      size_insertions2 = 0;
      size_deletions2 = 0;
      last_equality = thisDiff->text;
      has_last_equality = true;
      ++thisDiff;
    } else {
      // An insertion or deletion.
      if (thisDiff->operation == INSERT) {
        size_insertions2 += thisDiff->text.size();
      } else {
        size_deletions2 += thisDiff->text.size();
      }
      // Eliminate an equality that is smaller or equal to the edits on both
      // sides of it.
      if (has_last_equality && (last_equality.size() <=
                                std::max(size_insertions1, size_deletions1)) &&
          (last_equality.size() <=
           std::max(size_insertions2, size_deletions2))) {
        // printf("Splitting: '%s'\n", qPrintable(last_equality));
        // Walk back to offending equality.
        thisDiff = equalities.back();
        // Replace equality with a delete.
        *thisDiff = Diff(DELETE, last_equality);
        // Insert a corresponding an insert.
        auto it = thisDiff;
        ++it;
        diffs.insert(it, Diff(INSERT, last_equality));

        equalities.pop_back();  // Throw away the equality we just deleted.
        if (!equalities.empty()) {
          // Throw away the previous equality (it needs to be reevaluated).
          equalities.pop_back();
        }
        if (equalities.empty()) {
          thisDiff = diffs.begin();
        } else {
          // There is a safe equality we can fall back to.
          thisDiff = equalities.back();
        }

        size_insertions1 = 0;  // Reset the counters.
        size_deletions1 = 0;
        size_insertions2 = 0;
        size_deletions2 = 0;
        has_last_equality = false;
        changes = true;
      } else {
        ++thisDiff;
      }
    }
  }

  // Normalize the diff.
  if (changes) {
    diff_cleanupMerge(diffs);
  }
  diff_cleanupSemanticLossless(diffs);

  // Find any overlaps between deletions and insertions.
  // e.g: <del>abcxxx</del><ins>xxxdef</ins>
  //   -> <del>abc</del>xxx<ins>def</ins>
  // e.g: <del>xxxabc</del><ins>defxxx</ins>
  //   -> <ins>def</ins>xxx<del>abc</del>
  // Only extract an overlap if it is as big as the edit ahead or behind it.
  auto thisDiff = diffs.begin();
  auto prevDiff = (thisDiff != diffs.end()) ? thisDiff++ : diffs.end();
  while (thisDiff != diffs.end()) {
    if (prevDiff->operation == DELETE && thisDiff->operation == INSERT) {
      std::wstring deletion = prevDiff->text;
      std::wstring insertion = thisDiff->text;
      std::size_t overlap_size1 = diff_commonOverlap(deletion, insertion);
      std::size_t overlap_size2 = diff_commonOverlap(insertion, deletion);
      if (overlap_size1 >= overlap_size2) {
        if (overlap_size1 >= deletion.size() / 2.0 ||
            overlap_size1 >= insertion.size() / 2.0) {
          // Overlap found.  Insert an equality and trim the surrounding edits.
          diffs.insert(thisDiff,
                       Diff(EQUAL, insertion.substr(0, overlap_size1)));
          prevDiff->text = deletion.substr(0, deletion.size() - overlap_size1);
          thisDiff->text = safeSubStr(insertion, overlap_size1);
          // diffs.insert inserts the element before the cursor, so there is
          // no need to step past the new element.
        }
      } else {
        if (overlap_size2 >= deletion.size() / 2.0 ||
            overlap_size2 >= insertion.size() / 2.0) {
          // Reverse overlap found.
          // Insert an equality and swap and trim the surrounding edits.
          diffs.insert(thisDiff,
                       Diff(EQUAL, deletion.substr(0, overlap_size2)));
          prevDiff->operation = INSERT;
          prevDiff->text =
              insertion.substr(0, insertion.size() - overlap_size2);
          thisDiff->operation = DELETE;
          thisDiff->text = safeSubStr(deletion, overlap_size2);
          // pointer.insert inserts the element before the cursor, so there is
          // no need to step past the new element.
        }
      }
      ++thisDiff;
    }
    prevDiff = thisDiff;
    if (thisDiff != diffs.end()) ++thisDiff;
  }
}

void diff_match_patch::diff_cleanupSemanticLossless(std::list<Diff> &diffs) {
  std::wstring equality1, edit, equality2;
  std::wstring commonString;
  std::size_t commonOffset;
  int score, bestScore;
  std::wstring bestEquality1, bestEdit, bestEquality2;
  // Create a new iterator at the start.
  auto ptr = diffs.begin();
  auto prevDiff = ptr;
  auto thisDiff = (ptr != diffs.end()) ? ++ptr : ptr;
  auto nextDiff = (ptr != diffs.end()) ? ++ptr : ptr;

  // Intentionally ignore the first and last element (don't need checking).
  while (nextDiff != diffs.end()) {
    if (prevDiff->operation == EQUAL && nextDiff->operation == EQUAL) {
      // This is a single edit surrounded by equalities.
      equality1 = prevDiff->text;
      edit = thisDiff->text;
      equality2 = nextDiff->text;

      // First, shift the edit as far left as possible.
      commonOffset = diff_commonSuffix(equality1, edit);
      if (commonOffset != 0) {
        commonString = safeSubStr(edit, edit.size() - commonOffset);
        equality1 = equality1.substr(0, equality1.size() - commonOffset);
        edit = commonString + edit.substr(0, edit.size() - commonOffset);
        equality2 = commonString + equality2;
      }

      // Second, step character by character right, looking for the best fit.
      bestEquality1 = equality1;
      bestEdit = edit;
      bestEquality2 = equality2;
      bestScore = diff_cleanupSemanticScore(equality1, edit) +
                  diff_cleanupSemanticScore(edit, equality2);
      while (!edit.empty() && !equality2.empty() && edit[0] == equality2[0]) {
        equality1 += edit[0];
        edit = safeSubStr(edit, 1) + equality2[0];
        equality2 = safeSubStr(equality2, 1);
        score = diff_cleanupSemanticScore(equality1, edit) +
                diff_cleanupSemanticScore(edit, equality2);
        // The >= encourages trailing rather than leading whitespace on edits.
        if (score >= bestScore) {
          bestScore = score;
          bestEquality1 = equality1;
          bestEdit = edit;
          bestEquality2 = equality2;
        }
      }

      if (prevDiff->text != bestEquality1) {
        // We have an improvement, save it back to the diff.
        if (!bestEquality1.empty()) {
          prevDiff->text = bestEquality1;
        } else {
          diffs.erase(prevDiff);
        }
        thisDiff->text = bestEdit;
        if (!bestEquality2.empty()) {
          nextDiff->text = bestEquality2;
        } else {
          nextDiff = diffs.erase(nextDiff);
          nextDiff = thisDiff;
          thisDiff = prevDiff;
        }
      }
    }
    prevDiff = thisDiff;
    thisDiff = nextDiff;
    if (nextDiff != diffs.end()) ++nextDiff;
  }
}

int diff_match_patch::diff_cleanupSemanticScore(const std::wstring &one,
                                                const std::wstring &two) {
  if (one.empty() || two.empty()) {
    // Edges are the best.
    return 6;
  }

  // Each port of this function behaves slightly differently due to
  // subtle differences in each language's definition of things like
  // 'whitespace'.  Since this function's purpose is largely cosmetic,
  // the choice has been made to use each language's native features
  // rather than force total conformity.
  wchar_t char1 = one[one.size() - 1];
  wchar_t char2 = two[0];
  bool nonAlphaNumeric1 = !iswalnum(char1);
  bool nonAlphaNumeric2 = !iswalnum(char2);
  bool whitespace1 = nonAlphaNumeric1 && iswspace(char1);
  bool whitespace2 = nonAlphaNumeric2 && iswspace(char2);
  bool lineBreak1 = whitespace1 && iswcntrl(char1);
  bool lineBreak2 = whitespace2 && iswcntrl(char2);
  bool blankLine1 = lineBreak1 && std::regex_search(one, BLANKLINEEND);
  bool blankLine2 = lineBreak2 && std::regex_search(two, BLANKLINESTART);

  if (blankLine1 || blankLine2) {
    // Five points for blank lines.
    return 5;
  } else if (lineBreak1 || lineBreak2) {
    // Four points for line breaks.
    return 4;
  } else if (nonAlphaNumeric1 && !whitespace1 && whitespace2) {
    // Three points for end of sentences.
    return 3;
  } else if (whitespace1 || whitespace2) {
    // Two points for whitespace.
    return 2;
  } else if (nonAlphaNumeric1 || nonAlphaNumeric2) {
    // One point for non-alphanumeric.
    return 1;
  }
  return 0;
}

// Define some regex patterns for matching boundaries.
std::wregex diff_match_patch::BLANKLINEEND(L"\\n\\r?\\n$");
std::wregex diff_match_patch::BLANKLINESTART(L"^\\r?\\n\\r?\\n");

void diff_match_patch::diff_cleanupEfficiency(std::list<Diff> &diffs) {
  if (diffs.empty()) {
    return;
  }
  bool changes = false;
  std::vector<std::list<Diff>::iterator> equalities;  // Stack of equalities.
  bool has_last_equality=false;
  std::wstring last_equality;  // Always equal to equalities.lastElement().text
  // Is there an insertion operation before the last equality.
  bool pre_ins = false;
  // Is there a deletion operation before the last equality.
  bool pre_del = false;
  // Is there an insertion operation after the last equality.
  bool post_ins = false;
  // Is there a deletion operation after the last equality.
  bool post_del = false;

  auto thisDiff = diffs.begin();
  auto safeDiff = thisDiff;

  while (thisDiff != diffs.end()) {
    if (thisDiff->operation == EQUAL) {
      // Equality found.
      if (thisDiff->text.size() < Diff_EditCost && (post_ins || post_del)) {
        // Candidate found.
        equalities.push_back(thisDiff);
        pre_ins = post_ins;
        pre_del = post_del;
        last_equality = thisDiff->text;
        has_last_equality = true;
      } else {
        // Not a candidate, and can never become one.
        equalities.clear();
        has_last_equality = false;
        safeDiff = thisDiff;
      }
      post_ins = post_del = false;
      ++thisDiff;
    } else {
      // An insertion or deletion.
      if (thisDiff->operation == DELETE) {
        post_del = true;
      } else {
        post_ins = true;
      }
      /*
      * Five types to be split:
      * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
      * <ins>A</ins>X<ins>C</ins><del>D</del>
      * <ins>A</ins><del>B</del>X<ins>C</ins>
      * <ins>A</del>X<ins>C</ins><del>D</del>
      * <ins>A</ins><del>B</del>X<del>C</del>
      */
      if (has_last_equality &&
          ((pre_ins && pre_del && post_ins && post_del) ||
           ((last_equality.size() < Diff_EditCost / 2) &&
            ((pre_ins ? 1 : 0) + (pre_del ? 1 : 0) + (post_ins ? 1 : 0) +
             (post_del ? 1 : 0)) == 3))) {
        // printf("Splitting: '%s'\n", qPrintable(last_equality));
        // Walk back to offending equality.
        thisDiff = equalities.back();

        // Replace equality with a delete.
        *thisDiff = Diff(DELETE, last_equality);
        // Insert a corresponding an insert.
        auto it = thisDiff;
        ++it;
        thisDiff = diffs.insert(it, Diff(INSERT, last_equality));

        equalities.pop_back();  // Throw away the equality we just deleted.
        has_last_equality = false;
        if (pre_ins && pre_del) {
          // No changes made which could affect previous entry, keep going.
          post_ins = post_del = true;
          equalities.clear();
          safeDiff = thisDiff;
          ++thisDiff;
        } else {
          if (!equalities.empty()) {
            // Throw away the previous equality (it needs to be reevaluated).
            equalities.pop_back();
          }
          if (equalities.empty()) {
            // There are no previous questionable equalities,
            // walk back to the last known safe diff.
            thisDiff = safeDiff;
          } else {
            // There is an equality we can fall back to.
            thisDiff = equalities.back();
          }
          post_ins = post_del = false;
        }

        changes = true;
      } else {
        ++thisDiff;
      }
    }
  }

  if (changes) {
    diff_cleanupMerge(diffs);
  }
}

void diff_match_patch::diff_cleanupMerge(std::list<Diff> &diffs) {
  diffs.push_back(Diff(EQUAL, L""));  // Add a dummy entry at the end.
  std::size_t count_delete = 0;
  std::size_t count_insert = 0;
  std::wstring text_delete = L"";
  std::wstring text_insert = L"";
  Diff *prevEqual = NULL;
  std::size_t commonsize;
  for (auto thisDiff = diffs.begin(); thisDiff != diffs.end(); ++thisDiff) {
    switch (thisDiff->operation) {
      case INSERT:
        count_insert++;
        text_insert += thisDiff->text;
        prevEqual = NULL;
        break;
      case DELETE:
        count_delete++;
        text_delete += thisDiff->text;
        prevEqual = NULL;
        break;
      case EQUAL:
        if (count_delete + count_insert > 1) {
          bool both_types = count_delete != 0 && count_insert != 0;
          // Delete the offending records.
          auto it = thisDiff;
          while (count_delete-- > 0) {
            --it;
            it = diffs.erase(it);
          }
          while (count_insert-- > 0) {
            --it;
            it = diffs.erase(it);
          }
          if (both_types) {
            // Factor out any common prefixies.
            commonsize = diff_commonPrefix(text_insert, text_delete);
            if (commonsize != 0) {
              if (it != diffs.begin()) {
                --it;
                if (it->operation != EQUAL) {
                  throw "Previous diff should have been an equality.";
                }
                it->text += text_insert.substr(0, commonsize);
                ++it;
              } else {
                diffs.insert(it,
                             Diff(EQUAL, text_insert.substr(0, commonsize)));
              }
              text_insert = safeSubStr(text_insert, commonsize);
              text_delete = safeSubStr(text_delete, commonsize);
            }
            // Factor out any common suffixies.
            commonsize = diff_commonSuffix(text_insert, text_delete);
            if (commonsize != 0) {
              thisDiff->text =
                  safeSubStr(text_insert, text_insert.size() - commonsize) +
                  thisDiff->text;
              text_insert =
                  text_insert.substr(0, text_insert.size() - commonsize);
              text_delete =
                  text_delete.substr(0, text_delete.size() - commonsize);
            }
          }
          // Insert the merged records.
          if (!text_delete.empty()) {
            diffs.insert(thisDiff, Diff(DELETE, text_delete));
          }
          if (!text_insert.empty()) {
            diffs.insert(thisDiff, Diff(INSERT, text_insert));
          }
        } else if (prevEqual != NULL) {
          // Merge this equality with the previous one.
          prevEqual->text += thisDiff->text;
          thisDiff = diffs.erase(thisDiff);
          --thisDiff;
        }
        count_insert = 0;
        count_delete = 0;
        text_delete = L"";
        text_insert = L"";
        prevEqual = &(*thisDiff);
        break;
    }
  }
  if (diffs.back().text.empty()) {
    diffs.pop_back();  // Remove the dummy entry at the end.
  }

  /*
  * Second pass: look for single edits surrounded on both sides by equalities
  * which can be shifted sideways to eliminate an equality.
  * e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
  */
  bool changes = false;
  // Create a new iterator at the start.
  // (As opposed to walking the current one back.)
  auto thisDiff = diffs.begin();
  auto prevDiff = (thisDiff != diffs.end()) ? thisDiff++ : diffs.end();
  auto nextDiff = thisDiff;
  if (nextDiff != diffs.end()) ++nextDiff;

  // Intentionally ignore the first and last element (don't need checking).
  while (nextDiff != diffs.end()) {
    if (prevDiff->operation == EQUAL && nextDiff->operation == EQUAL) {
      // This is a single edit surrounded by equalities.
      if (EndsWith(thisDiff->text, prevDiff->text)) {
        // Shift the edit over the previous equality.
        thisDiff->text = prevDiff->text +
                         thisDiff->text.substr(
                             0, thisDiff->text.size() - prevDiff->text.size());
        nextDiff->text = prevDiff->text + nextDiff->text;
        // Delete prevDiff.
        diffs.erase(prevDiff);
        ++thisDiff;
        ++nextDiff;
        changes = true;
      } else if (StartsWith(thisDiff->text, nextDiff->text)) {
        // Shift the edit over the next equality.
        prevDiff->text += nextDiff->text;
        thisDiff->text =
            safeSubStr(thisDiff->text, nextDiff->text.size()) + nextDiff->text;
        nextDiff = diffs.erase(nextDiff);
        changes = true;
      }
    }
    prevDiff = thisDiff;
    thisDiff = nextDiff;
    if (nextDiff != diffs.end()) ++nextDiff;
  }
  // If shifts were made, the diff needs reordering and another shift sweep.
  if (changes) {
    diff_cleanupMerge(diffs);
  }
}

std::size_t diff_match_patch::diff_xIndex(const std::list<Diff> &diffs,
                                          std::size_t loc) {
  std::size_t chars1 = 0;
  std::size_t chars2 = 0;
  std::size_t last_chars1 = 0;
  std::size_t last_chars2 = 0;
  Diff lastDiff;
  for (const auto &aDiff : diffs) {
    if (aDiff.operation != INSERT) {
      // Equality or deletion.
      chars1 += aDiff.text.size();
    }
    if (aDiff.operation != DELETE) {
      // Equality or insertion.
      chars2 += aDiff.text.size();
    }
    if (chars1 > loc) {
      // Overshot the location.
      lastDiff = aDiff;
      break;
    }
    last_chars1 = chars1;
    last_chars2 = chars2;
  }
  if (lastDiff.operation == DELETE) {
    // The location was deleted.
    return last_chars2;
  }
  // Add the remaining character size.
  return last_chars2 + (loc - last_chars1);
}

std::string diff_match_patch::diff_prettyHtml(const std::list<Diff> &diffs) {
  UnicodeEncoder unicode_encoder;
  return unicode_encoder.to_bytes(diff_widePrettyHtml(diffs));
}

std::wstring diff_match_patch::diff_widePrettyHtml(
    const std::list<Diff> &diffs) {
  std::wstring html;
  std::wstring text;
  for (const auto &aDiff : diffs) {
    text = aDiff.text;
    ReplaceAll(text, L'&', L"&amp;");
    ReplaceAll(text, L'<', L"&lt;");
    ReplaceAll(text, L'>', L"&gt;");
    ReplaceAll(text, L'\n', L"&para;<br>");
    switch (aDiff.operation) {
      case INSERT:
        html += L"<ins style=\"background:#e6ffe6;\">" + text + L"</ins>";
        break;
      case DELETE:
        html += L"<del style=\"background:#ffe6e6;\">" + text + L"</del>";
        break;
      case EQUAL:
        html += L"<span>" + text + L"</span>";
        break;
    }
  }
  return html;
}

std::string diff_match_patch::diff_text1(const std::list<Diff> &diffs) {
  UnicodeEncoder unicode_encoder;
  return unicode_encoder.to_bytes(diff_wideText1(diffs));
}

std::wstring diff_match_patch::diff_wideText1(const std::list<Diff> &diffs) {
  std::wstring text;
  for (const auto &aDiff : diffs) {
    if (aDiff.operation != INSERT) {
      text += aDiff.text;
    }
  }
  return text;
}

std::string diff_match_patch::diff_text2(const std::list<Diff> &diffs) {
  UnicodeEncoder unicode_encoder;
  return unicode_encoder.to_bytes(diff_wideText2(diffs));
}

std::wstring diff_match_patch::diff_wideText2(const std::list<Diff> &diffs) {
  std::wstring text;
  for (const auto &aDiff : diffs) {
    if (aDiff.operation != DELETE) {
      text += aDiff.text;
    }
  }
  return text;
}

std::size_t diff_match_patch::diff_levenshtein(const std::list<Diff> &diffs) {
  std::size_t levenshtein = 0;
  std::size_t insertions = 0;
  std::size_t deletions = 0;
  for (const auto &aDiff : diffs) {
    switch (aDiff.operation) {
      case INSERT:
        insertions += aDiff.text.size();
        break;
      case DELETE:
        deletions += aDiff.text.size();
        break;
      case EQUAL:
        // A deletion and an insertion is one substitution.
        levenshtein += std::max(insertions, deletions);
        insertions = 0;
        deletions = 0;
        break;
    }
  }
  levenshtein += std::max(insertions, deletions);
  return levenshtein;
}

std::string diff_match_patch::diff_toDelta(const std::list<Diff> &diffs) {
  UnicodeEncoder unicode_encoder;
  return unicode_encoder.to_bytes(diff_toWideDelta(diffs));
}

std::wstring diff_match_patch::diff_toWideDelta(const std::list<Diff> &diffs) {
  std::wstringstream text;
  for (const auto &aDiff : diffs) {
    switch (aDiff.operation) {
      case INSERT: {
        text << L'+' << URLEncode(aDiff.text, L" !~*'();/?:@&=+$,#") << L'\t';
        break;
      }
      case DELETE:
        text << L'-' << aDiff.text.size() << '\t';
        break;
      case EQUAL:
        text << L'=' << aDiff.text.size() << L'\t';
        break;
    }
  }
  const auto &res = text.str();
  if (!res.empty()) {
    // Strip off trailing tab character.
    return res.substr(0, res.size() - 1);
  }
  return res;
}

std::list<Diff> diff_match_patch::diff_fromDelta(const std::string &text1,
                                                 const std::string &delta) {
  UnicodeEncoder unicode_encoder;
  return diff_fromDelta(unicode_encoder.from_bytes(text1),
                        unicode_encoder.from_bytes(delta));
}

std::list<Diff> diff_match_patch::diff_fromDelta(const std::wstring &text1,
                                                 const std::wstring &delta) {
  std::list<Diff> diffs;
  std::size_t pointer = 0;  // Cursor in text1
  std::vector<std::wstring> tokens;
  Split(delta, '\t', tokens);
  for (const auto &token : tokens) {
    if (token.empty()) {
      // Blank tokens are ok (from a trailing \t).
      continue;
    }
    // Each token begins with a one character parameter which specifies the
    // operation of this token (delete, insert, equality).
    std::wstring param = safeSubStr(token, 1);
    switch (token[0]) {
      case L'+':
        param = URLDecode(param);
        diffs.push_back(Diff(INSERT, param));
        break;
      case L'-':
      // Fall through.
      case L'=': {
        int64_t n = AsInt64(param);
        if (n < 0) {
          UnicodeEncoder unicode_encoder;
          throw "Negative number in diff_fromDelta: " +
              unicode_encoder.to_bytes(param);
        }
        std::wstring text;
        text = safeSubStr(text1, pointer, n);
        pointer += n;
        if (token[0] == L'=') {
          diffs.push_back(Diff(EQUAL, text));
        } else {
          diffs.push_back(Diff(DELETE, text));
        }
        break;
      }
      default: {
        UnicodeEncoder unicode_encoder;
        throw "Invalid diff operation in diff_fromDelta: " +
            unicode_encoder.to_bytes(std::wstring(1, token[0]));
      }
    }
  }
  if (pointer != text1.size()) {
    throw "Delta size (" + AsString(pointer) +
        ") smaller than source text size (" + AsString(text1.size()) + ")";
  }
  return diffs;
}

//  MATCH FUNCTIONS

std::size_t diff_match_patch::match_main(const std::string &text,
                                         const std::string &pattern,
                                         std::size_t loc) {
  UnicodeEncoder unicode_encoder;
  return match_main(unicode_encoder.from_bytes(text),
                    unicode_encoder.from_bytes(pattern), loc);
}

std::size_t diff_match_patch::match_main(const std::wstring &text,
                                         const std::wstring &pattern,
                                         std::size_t loc) {
  loc = std::max(0UL, (unsigned long) std::min(loc, text.size()));
  if (text == pattern) {
    // Shortcut (potentially not guaranteed by the algorithm)
    return 0;
  } else if (text.empty()) {
    // Nothing to match.
    return std::wstring::npos;
  } else if (loc + pattern.size() <= text.size() &&
             safeSubStr(text, loc, pattern.size()) == pattern) {
    // Perfect match at the perfect spot!  (Includes case of null pattern)
    return loc;
  } else {
    // Do a fuzzy compare.
    return match_bitap(text, pattern, loc);
  }
}

std::size_t diff_match_patch::match_bitap(const std::wstring &text,
                                          const std::wstring &pattern,
                                          std::size_t loc) {
  if (!(Match_MaxBits == 0 || pattern.size() <= Match_MaxBits)) {
    throw "Pattern too long for this application.";
  }

  // Initialise the alphabet.
  auto s = match_alphabet(pattern);

  // Highest score beyond which we give up.
  double score_threshold = Match_Threshold;
  // Is there a nearby exact match? (speedup)
  std::size_t best_loc = text.find(pattern, loc);
  if (best_loc != std::wstring::npos) {
    score_threshold =
        std::min(match_bitapScore(0, best_loc, loc, pattern), score_threshold);
    // What about in the other direction? (speedup)
    best_loc = text.rfind(pattern, loc + pattern.size());
    if (best_loc != std::wstring::npos) {
      score_threshold = std::min(match_bitapScore(0, best_loc, loc, pattern),
                                 score_threshold);
    }
  }

  // Initialise the bit arrays.
  std::size_t matchmask = 1 << (pattern.size() - 1);
  best_loc = std::wstring::npos;

  std::size_t bin_min, bin_mid;
  std::size_t bin_max = pattern.size() + text.size();
  std::unique_ptr<std::size_t[]> rd;
  std::unique_ptr<std::size_t[]> last_rd = NULL;
  for (std::size_t d = 0; d < pattern.size(); d++) {
    // Scan for the best match; each iteration allows for one more error.
    // Run a binary search to determine how far from 'loc' we can stray at
    // this error level.
    bin_min = 0;
    bin_mid = bin_max;
    while (bin_min < bin_mid) {
      if (match_bitapScore(d, loc + bin_mid, loc, pattern) <= score_threshold) {
        bin_min = bin_mid;
      } else {
        bin_max = bin_mid;
      }
      bin_mid = (bin_max - bin_min) / 2 + bin_min;
    }
    // Use the result from this iteration as the maximum for the next.
    bin_max = bin_mid;
    std::size_t start = std::max<int64_t>(1, (int64_t)loc - bin_mid + 1);
    std::size_t finish = std::min(loc + bin_mid, text.size()) + pattern.size();

    rd.reset(new std::size_t[finish + 2]);
    rd[finish + 1] = (1 << d) - 1;
    for (std::size_t j = finish; j >= start; j--) {
      std::size_t charMatch;
      if (text.size() <= j - 1) {
        // Out of range.
        charMatch = 0;
      } else {
        charMatch = s[text[j - 1]];
      }
      if (d == 0) {
        // First pass: exact match.
        rd[j] = ((rd[j + 1] << 1) | 1) & charMatch;
      } else {
        // Subsequent passes: fuzzy match.
        rd[j] = (((rd[j + 1] << 1) | 1) & charMatch) |
                (((last_rd[j + 1] | last_rd[j]) << 1) | 1) | last_rd[j + 1];
      }
      if ((rd[j] & matchmask) != 0) {
        double score = match_bitapScore(d, j - 1, loc, pattern);
        // This match will almost certainly be better than any existing
        // match.  But check anyway.
        if (score <= score_threshold) {
          // Told you so.
          score_threshold = score;
          best_loc = j - 1;
          if (best_loc > loc) {
            // When passing loc, don't exceed our current distance from loc.
            start = std::max<int64_t>(1, 2 * (int64_t)loc - best_loc);
          } else {
            // Already passed loc, downhill from here on in.
            break;
          }
        }
      }
    }
    if (match_bitapScore(d + 1, loc, loc, pattern) > score_threshold) {
      // No hope for a (better) match at greater error levels.
      break;
    }
    last_rd = std::move(rd);
  }
  return best_loc;
}

double diff_match_patch::match_bitapScore(std::size_t e, std::size_t x,
                                          std::size_t loc,
                                          const std::wstring &pattern) {
  const float accuracy = static_cast<float>(e) / pattern.size();
  const std::size_t proximity = (loc > x) ? (loc - x) : x - loc;
  if (Match_Distance == 0) {
    // Dodge divide by zero error.
    return proximity == 0 ? accuracy : 1.0;
  }
  return accuracy + (proximity / static_cast<float>(Match_Distance));
}

std::unordered_map<wchar_t, std::size_t> diff_match_patch::match_alphabet(
    const std::wstring &pattern) {
  std::unordered_map<wchar_t, std::size_t> s;
  std::size_t i;
  for (auto c : pattern) {
    s.emplace(c, 0);
  }
  std::size_t mask = 1 << (pattern.size() - 1);
  for (auto c : pattern) {
    s[c] |= mask;
    mask >>= 1;
  }
  return s;
}

//  PATCH FUNCTIONS

void diff_match_patch::patch_addContext(Patch &patch,
                                        const std::wstring &text) {
  if (text.empty()) {
    return;
  }
  std::wstring pattern = safeSubStr(text, patch.start2, patch.size1);
  std::size_t padding = 0;

  // Look for the first and last matches of pattern in text.  If two different
  // matches are found, increase the pattern size.
  while (text.find(pattern) != text.rfind(pattern) &&
         pattern.size() < Match_MaxBits - Patch_Margin - Patch_Margin) {
    padding += Patch_Margin;
    std::size_t offset = patch.start2 > padding ? patch.start2 - padding : 0;
    pattern = safeSubStr(
        text, offset,
        std::min(text.size(), patch.start2 + patch.size1 + padding) - offset);
  }
  // Add one chunk for good luck.
  padding += Patch_Margin;

  // Add the prefix.
  auto offset = patch.start2 > padding ? patch.start2 - padding : 0;
  std::wstring prefix = safeSubStr(text, offset, patch.start2 - offset);
  if (!prefix.empty()) {
    patch.diffs.push_front(Diff(EQUAL, prefix));
  }
  // Add the suffix.
  std::wstring suffix =
      safeSubStr(text, patch.start2 + patch.size1,
                 std::min(text.size(), patch.start2 + patch.size1 + padding) -
                     (patch.start2 + patch.size1));
  if (!suffix.empty()) {
    patch.diffs.push_back(Diff(EQUAL, suffix));
  }

  // Roll back the start points.
  patch.start1 -= prefix.size();
  patch.start2 -= prefix.size();
  // Extend the sizes.
  patch.size1 += prefix.size() + suffix.size();
  patch.size2 += prefix.size() + suffix.size();
}

std::list<Patch> diff_match_patch::patch_make(const std::string &text1,
                                              const std::string &text2) {
  UnicodeEncoder unicode_encoder;
  return patch_make(unicode_encoder.from_bytes(text1),
                    unicode_encoder.from_bytes(text2));
}

std::list<Patch> diff_match_patch::patch_make(const std::wstring &text1,
                                              const std::wstring &text2) {
  // No diffs provided, compute our own.
  std::list<Diff> diffs = diff_main(text1, text2, true);
  if (diffs.size() > 2) {
    diff_cleanupSemantic(diffs);
    diff_cleanupEfficiency(diffs);
  }

  return patch_make(text1, diffs);
}

std::list<Patch> diff_match_patch::patch_make(const std::list<Diff> &diffs) {
  // No origin string provided, compute our own.
  const std::wstring text1 = diff_wideText1(diffs);
  return patch_make(text1, diffs);
}

std::list<Patch> diff_match_patch::patch_make(const std::string &text1,
                                              const std::string & /*text2*/,
                                              const std::list<Diff> &diffs) {
  // text2 is entirely unused.
  UnicodeEncoder unicode_encoder;
  return patch_make(unicode_encoder.from_bytes(text1), diffs);
}

std::list<Patch> diff_match_patch::patch_make(const std::wstring &text1,
                                              const std::wstring & /*text2*/,
                                              const std::list<Diff> &diffs) {
  // text2 is entirely unused.
  return patch_make(text1, diffs);
}

std::list<Patch> diff_match_patch::patch_make(const std::string &text1,
                                              const std::list<Diff> &diffs) {
  UnicodeEncoder unicode_encoder;
  return patch_make(unicode_encoder.from_bytes(text1), diffs);
}

std::list<Patch> diff_match_patch::patch_make(const std::wstring &text1,
                                              const std::list<Diff> &diffs) {
  std::list<Patch> patches;
  if (diffs.empty()) {
    return patches;  // Get rid of the null case.
  }
  Patch patch;
  std::size_t char_count1 = 0;  // Number of characters into the text1 string.
  std::size_t char_count2 = 0;  // Number of characters into the text2 string.
  // Start with text1 (prepatch_text) and apply the diffs until we arrive at
  // text2 (postpatch_text).  We recreate the patches one by one to determine
  // context info.
  std::wstring prepatch_text = text1;
  std::wstring postpatch_text = text1;
  for (const auto &aDiff : diffs) {
    if (patch.diffs.empty() && aDiff.operation != EQUAL) {
      // A new patch starts here.
      patch.start1 = char_count1;
      patch.start2 = char_count2;
    }

    switch (aDiff.operation) {
      case INSERT:
        patch.diffs.push_back(aDiff);
        patch.size2 += aDiff.text.size();
        postpatch_text = postpatch_text.substr(0, char_count2) + aDiff.text +
                         safeSubStr(postpatch_text, char_count2);
        break;
      case DELETE:
        patch.size1 += aDiff.text.size();
        patch.diffs.push_back(aDiff);
        postpatch_text =
            postpatch_text.substr(0, char_count2) +
            safeSubStr(postpatch_text, char_count2 + aDiff.text.size());
        break;
      case EQUAL:
        if (aDiff.text.size() <= 2 * Patch_Margin && !patch.diffs.empty() &&
            !(aDiff == diffs.back())) {
          // Small equality inside a patch.
          patch.diffs.push_back(aDiff);
          patch.size1 += aDiff.text.size();
          patch.size2 += aDiff.text.size();
        }

        if (aDiff.text.size() >= 2 * Patch_Margin) {
          // Time for a new patch.
          if (!patch.diffs.empty()) {
            patch_addContext(patch, prepatch_text);
            patches.push_back(patch);
            patch = Patch();
            // Unlike Unidiff, our patch lists have a rolling context.
            // http://code.google.com/p/google-diff-match-patch/wiki/Unidiff
            // Update prepatch text & pos to reflect the application of the
            // just completed patch.
            prepatch_text = postpatch_text;
            char_count1 = char_count2;
          }
        }
        break;
    }

    // Update the current character count.
    if (aDiff.operation != INSERT) {
      char_count1 += aDiff.text.size();
    }
    if (aDiff.operation != DELETE) {
      char_count2 += aDiff.text.size();
    }
  }
  // Pick up the leftover patch if not empty.
  if (!patch.diffs.empty()) {
    patch_addContext(patch, prepatch_text);
    patches.push_back(patch);
  }

  return patches;
}

std::list<Patch> diff_match_patch::patch_deepCopy(
    const std::list<Patch> &patches) {
  std::list<Patch> patchesCopy;
  for (const auto &aPatch : patches) {
    Patch patchCopy = Patch();
    for (const auto &aDiff : aPatch.diffs) {
      Diff diffCopy = Diff(aDiff.operation, aDiff.text);
      patchCopy.diffs.push_back(diffCopy);
    }
    patchCopy.start1 = aPatch.start1;
    patchCopy.start2 = aPatch.start2;
    patchCopy.size1 = aPatch.size1;
    patchCopy.size2 = aPatch.size2;
    patchesCopy.push_back(patchCopy);
  }
  return patchesCopy;
}

std::pair<std::string, std::vector<bool> > diff_match_patch::patch_apply(
    const std::list<Patch> &patches, const std::string &text) {
  UnicodeEncoder unicode_encoder;
  auto wide_result = patch_apply(patches, unicode_encoder.from_bytes(text));
  return std::make_pair(unicode_encoder.to_bytes(wide_result.first),
                        wide_result.second);
}

std::pair<std::wstring, std::vector<bool> > diff_match_patch::patch_apply(
    const std::list<Patch> &patches, const std::wstring &sourceText) {
  std::wstring text = sourceText;  // Copy to preserve original.
  if (patches.empty()) {
    return std::pair<std::wstring, std::vector<bool> >(text,
                                                       std::vector<bool>(0));
  }

  // Deep copy the patches so that no changes are made to originals.
  auto patchesCopy = patch_deepCopy(patches);

  std::wstring nullPadding = patch_addWidePadding(patchesCopy);
  text = nullPadding + text + nullPadding;
  patch_splitMax(patchesCopy);

  std::size_t x = 0;
  // delta keeps track of the offset between the expected and actual location
  // of the previous patch.  If there are patches expected at positions 10 and
  // 20, but the first patch was found at 12, delta is 2 and the second patch
  // has an effective expected position of 22.
  std::size_t delta = 0;
  std::vector<bool> results(patchesCopy.size());
  for (const auto &aPatch : patchesCopy) {
    std::size_t expected_loc = aPatch.start2 + delta;
    std::wstring text1 = diff_wideText1(aPatch.diffs);
    std::size_t start_loc;
    std::size_t end_loc = std::wstring::npos;
    if (text1.size() > Match_MaxBits) {
      // patch_splitMax will only provide an oversized pattern in the case of
      // a monster delete.
      start_loc =
          match_main(text, text1.substr(0, Match_MaxBits), expected_loc);
      if (start_loc != std::wstring::npos) {
        end_loc = match_main(text, text1.substr(text1.size() - Match_MaxBits),
                             expected_loc + text1.size() - Match_MaxBits);
        if (end_loc == std::wstring::npos || start_loc >= end_loc) {
          // Can't find valid trailing context.  Drop this patch.
          start_loc = std::wstring::npos;
        }
      }
    } else {
      start_loc = match_main(text, text1, expected_loc);
    }
    if (start_loc == std::wstring::npos) {
      // No match found.  :(
      results[x] = false;
      // Subtract the delta for this failed patch from subsequent patches.
      delta -= aPatch.size2 - aPatch.size1;
    } else {
      // Found a match.  :)
      results[x] = true;
      delta = start_loc - expected_loc;
      std::wstring text2;
      if (end_loc == std::wstring::npos) {
        text2 = safeSubStr(text, start_loc, text1.size());
      } else {
        text2 =
            safeSubStr(text, start_loc, end_loc + Match_MaxBits - start_loc);
      }
      if (text1 == text2) {
        // Perfect match, just shove the replacement text in.
        text = text.substr(0, start_loc) + diff_wideText2(aPatch.diffs) +
               safeSubStr(text, start_loc + text1.size());
      } else {
        // Imperfect match.  Run a diff to get a framework of equivalent
        // indices.
        std::list<Diff> diffs = diff_main(text1, text2, false);
        if (text1.size() > Match_MaxBits &&
            diff_levenshtein(diffs) / static_cast<float>(text1.size()) >
                Patch_DeleteThreshold) {
          // The end points match, but the content is unacceptably bad.
          results[x] = false;
        } else {
          diff_cleanupSemanticLossless(diffs);
          std::size_t index1 = 0;
          for (const auto &aDiff : aPatch.diffs) {
            if (aDiff.operation != EQUAL) {
              std::size_t index2 = diff_xIndex(diffs, index1);
              if (aDiff.operation == INSERT) {
                // Insertion
                text = text.substr(0, start_loc + index2) + aDiff.text +
                       safeSubStr(text, start_loc + index2);
              } else if (aDiff.operation == DELETE) {
                // Deletion
                text = text.substr(0, start_loc + index2) +
                       safeSubStr(
                           text,
                           start_loc +
                               diff_xIndex(diffs, index1 + aDiff.text.size()));
              }
            }
            if (aDiff.operation != DELETE) {
              index1 += aDiff.text.size();
            }
          }
        }
      }
    }
    x++;
  }
  // Strip the padding off.
  text = safeSubStr(text, nullPadding.size(),
                    text.size() - 2 * nullPadding.size());
  return std::pair<std::wstring, std::vector<bool> >(text, results);
}

std::string diff_match_patch::patch_addPadding(std::list<Patch> &patches) {
  UnicodeEncoder unicode_encoder;
  return unicode_encoder.to_bytes(patch_addWidePadding(patches));
}

std::wstring diff_match_patch::patch_addWidePadding(std::list<Patch> &patches) {
  short paddingLength = Patch_Margin;
  std::wstring nullPadding = L"";
  for (short x = 1; x <= paddingLength; x++) {
    nullPadding += wchar_t((ushort)x);
  }

  // Bump all the patches forward.
  for (auto &patch : patches) {
    patch.start1 += paddingLength;
    patch.start2 += paddingLength;
  }

  // Add some padding on start of first diff.
  Patch &firstPatch = patches.front();
  std::list<Diff> &firstPatchDiffs = firstPatch.diffs;
  if (firstPatchDiffs.empty() || firstPatchDiffs.front().operation != EQUAL) {
    // Add nullPadding equality.
    firstPatchDiffs.push_front(Diff(EQUAL, nullPadding));
    firstPatch.start1 -= paddingLength;  // Should be 0.
    firstPatch.start2 -= paddingLength;  // Should be 0.
    firstPatch.size1 += paddingLength;
    firstPatch.size2 += paddingLength;
  } else if (paddingLength > firstPatchDiffs.front().text.size()) {
    // Grow first equality.
    Diff &firstDiff = firstPatchDiffs.front();
    std::size_t extraLength = paddingLength - firstDiff.text.size();
    firstDiff.text = safeSubStr(nullPadding, firstDiff.text.size(),
                                paddingLength - firstDiff.text.size()) +
                     firstDiff.text;
    firstPatch.start1 -= extraLength;
    firstPatch.start2 -= extraLength;
    firstPatch.size1 += extraLength;
    firstPatch.size2 += extraLength;
  }

  // Add some padding on end of last diff.
  Patch &lastPatch = patches.front();
  std::list<Diff> &lastPatchDiffs = lastPatch.diffs;
  if (lastPatchDiffs.empty() || lastPatchDiffs.back().operation != EQUAL) {
    // Add nullPadding equality.
    lastPatchDiffs.push_back(Diff(EQUAL, nullPadding));
    lastPatch.size1 += paddingLength;
    lastPatch.size2 += paddingLength;
  } else if (paddingLength > lastPatchDiffs.back().text.size()) {
    // Grow last equality.
    Diff &lastDiff = lastPatchDiffs.back();
    std::size_t extraLength = paddingLength - lastDiff.text.size();
    lastDiff.text += nullPadding.substr(0, extraLength);
    lastPatch.size1 += extraLength;
    lastPatch.size2 += extraLength;
  }

  return nullPadding;
}

void diff_match_patch::patch_splitMax(std::list<Patch> &patches) {
  short patch_size = Match_MaxBits;
  std::wstring precontext, postcontext;
  Patch patch;
  std::size_t start1, start2;
  bool empty;
  Operation diff_type;
  std::wstring diff_text;
  auto bigpatch = patches.begin();
  for (; bigpatch != patches.end();) {
    if (bigpatch->size1 <= patch_size) {
      ++bigpatch;
      continue;
    }
    // Remove the big old patch.
    start1 = bigpatch->start1;
    start2 = bigpatch->start2;
    precontext = L"";
    while (!bigpatch->diffs.empty()) {
      // Create one of several smaller patches.
      patch = Patch();
      empty = true;
      patch.start1 = start1 - precontext.size();
      patch.start2 = start2 - precontext.size();
      if (!precontext.empty()) {
        patch.size1 = patch.size2 = precontext.size();
        patch.diffs.push_back(Diff(EQUAL, precontext));
      }
      while (!bigpatch->diffs.empty() &&
             patch.size1 < patch_size - Patch_Margin) {
        diff_type = bigpatch->diffs.front().operation;
        diff_text = bigpatch->diffs.front().text;
        if (diff_type == INSERT) {
          // Insertions are harmless.
          patch.size2 += diff_text.size();
          start2 += diff_text.size();
          patch.diffs.push_back(bigpatch->diffs.front());
          bigpatch->diffs.pop_front();
          empty = false;
        } else if (diff_type == DELETE && patch.diffs.size() == 1 &&
                   patch.diffs.front().operation == EQUAL &&
                   diff_text.size() > 2 * patch_size) {
          // This is a large deletion.  Let it pass in one chunk.
          patch.size1 += diff_text.size();
          start1 += diff_text.size();
          empty = false;
          patch.diffs.push_back(Diff(diff_type, diff_text));
          bigpatch->diffs.pop_front();
        } else {
          // Deletion or equality.  Only take as much as we can stomach.
          diff_text = diff_text.substr(
              0, std::min(diff_text.size(),
                          patch_size - patch.size1 - Patch_Margin));
          patch.size1 += diff_text.size();
          start1 += diff_text.size();
          if (diff_type == EQUAL) {
            patch.size2 += diff_text.size();
            start2 += diff_text.size();
          } else {
            empty = false;
          }
          patch.diffs.push_back(Diff(diff_type, diff_text));
          if (diff_text == bigpatch->diffs.front().text) {
            bigpatch->diffs.pop_front();
          } else {
            bigpatch->diffs.front().text =
                safeSubStr(bigpatch->diffs.front().text, diff_text.size());
          }
        }
      }
      // Compute the head context for the next patch.
      precontext = diff_wideText2(patch.diffs);
      precontext = safeSubStr(precontext, precontext.size() - Patch_Margin);
      // Append the end context for this patch.
      if (diff_wideText1(bigpatch->diffs).size() > Patch_Margin) {
        postcontext = diff_wideText1(bigpatch->diffs).substr(0, Patch_Margin);
      } else {
        postcontext = diff_wideText1(bigpatch->diffs);
      }
      if (!postcontext.empty()) {
        patch.size1 += postcontext.size();
        patch.size2 += postcontext.size();
        if (!patch.diffs.empty() && patch.diffs.back().operation == EQUAL) {
          patch.diffs.back().text += postcontext;
        } else {
          patch.diffs.push_back(Diff(EQUAL, postcontext));
        }
      }
      if (!empty) {
        patches.insert(bigpatch, patch);
      }
    }
    bigpatch = patches.erase(bigpatch);
  }
}

std::string diff_match_patch::patch_toText(const std::list<Patch> &patches) {
  UnicodeEncoder unicode_encoder;
  return unicode_encoder.to_bytes(patch_toWideText(patches));
}

std::wstring diff_match_patch::patch_toWideText(
    const std::list<Patch> &patches) {
  std::wstring text;
  for (const auto &aPatch : patches) {
    text += aPatch.toString();
  }
  return text;
}

std::list<Patch> diff_match_patch::patch_fromText(const std::string &textline) {
  UnicodeEncoder unicode_encoder;
  return patch_fromText(unicode_encoder.from_bytes(textline));
}

std::list<Patch> diff_match_patch::patch_fromText(
    const std::wstring &textline) {
  std::list<Patch> patches;
  if (textline.empty()) {
    return patches;
  }
  std::deque<std::wstring> text;
  Split(textline, '\n', text);
  Patch patch;
  std::wregex patchHeader(L"^@@ -(\\d+),?(\\d*) \\+(\\d+),?(\\d*) @@$");
  wchar_t sign;
  std::wstring line;
  while (!text.empty()) {
    if (text.front().empty()) {
      text.pop_front();
      continue;
    }
    std::wsmatch matches;
    if (!std::regex_match(text.front(), matches, patchHeader)) {
      UnicodeEncoder unicode_encoder;
      throw "Invalid patch string: " + unicode_encoder.to_bytes(text.front());
    }

    patch = Patch();
    patch.start1 = AsSizeT(matches[1].str());
    if (matches[2].length() == 0) {
      patch.start1--;
      patch.size1 = 1;
    } else if (matches[2].str() == L"0") {
      patch.size1 = 0;
    } else {
      patch.start1--;
      patch.size1 = AsSizeT(matches[2].str());
    }

    patch.start2 = AsSizeT(matches[3].str());
    if (matches[4].length() == 0) {
      patch.start2--;
      patch.size2 = 1;
    } else if (matches[4].str() == L"0") {
      patch.size2 = 0;
    } else {
      patch.start2--;
      patch.size2 = AsSizeT(matches[4].str());
    }
    text.pop_front();

    while (!text.empty()) {
      if (text.front().empty()) {
        text.pop_front();
        continue;
      }
      sign = text.front()[0];
      line = safeSubStr(text.front(), 1);
      ReplaceAll(line, L'+', L"%2B");  // decode would change all "+" to " "
      line = URLDecode(line);
      if (sign == L'-') {
        // Deletion.
        patch.diffs.push_back(Diff(DELETE, line));
      } else if (sign == L'+') {
        // Insertion.
        patch.diffs.push_back(Diff(INSERT, line));
      } else if (sign == L' ') {
        // Minor equality.
        patch.diffs.push_back(Diff(EQUAL, line));
      } else if (sign == L'@') {
        // Start of next patch.
        break;
      } else {
        // WTF?
        UnicodeEncoder unicode_encoder;
        throw "Invalid patch mode '" +
            unicode_encoder.to_bytes(std::wstring(1, sign)) + "' in: " +
            unicode_encoder.to_bytes(line);
      }
      text.pop_front();
    }

    patches.push_back(patch);
  }
  return patches;
}
