#include "HtmlGenerator.h"

String HtmlGenerator::inputBase(String type, String name, String val) {
  String resp = "<input type=\"" + type + "\" name=\"" + name + "\"";
  if (val.length() > 0) resp += " value=\"" + val + "\"";
  resp += ">";
  return resp;
}

String HtmlGenerator::labelBase(String txt, String name) {
  return "<label for=\"" + name + "\">" + txt + "</label>";
}

String HtmlGenerator::header(String title, String enc) {
  String resp;
  resp += "<!DOCTYPE html>";
  resp += "<html><head>";
  resp += "<meta charset=\"" + enc + "\">";
  resp += "<title>" + title + "</title>";
  resp += "<style> \
p, li, a { \
  font-size: 14px; \
} \
a { \
  text-decoration: none; \
} \
a:hover { \
  text-decoration: underline; \
} \
input, select { width: 100\%; \
} \
input[type=text], select { \
  padding: 6px 10px; \
  margin: 4px 0; \
  display: inline-block; \
  border: 1px solid #ccc; \
  border-radius: 8px; \
  box-sizing: border-box; \
} \
\
input[type=submit] { \
  background-color: #4CAF50; \
  color: white; \
  padding: 8px 10px; \
  margin: 4px 0; \
  border: none; \
  border-radius: 8px; \
  cursor: pointer; \
} \
\
input[type=submit]:hover { \
  background-color: #45a049; \
} \
\
div { \
  border-radius: 5px; \
  background-color: #f2f2f2; \
  padding: 5px; \
} \
</style>";
  resp += "</head><body>";
  return resp;
}

String HtmlGenerator::h(int level, String txt) {
  return "<h" + String(level) + String(">") + txt + "</h" + String(level) + String(">");
}

String HtmlGenerator::formStart(String meth, String action) {
  return "<form action=\"" + action + "\" method=\"" + meth + "\">";
}

String HtmlGenerator::formEnd() {
  return "</form>";
}

String HtmlGenerator::textinput(String label, String name, String val) {
  return HtmlGenerator::labelBase( label, name ) + HtmlGenerator::inputBase("text", name, val);
}

String HtmlGenerator::passwordinput(String label, String name) {
  return HtmlGenerator::labelBase( label, name ) + HtmlGenerator::inputBase("password", name);
}

String HtmlGenerator::button(String txt, String name, String type, String value) {
  String resp = "<button type=\"" + type + "\" name=\"" + name + "\"";
  if (value.length() > 0) {
    resp += " value=\"" + value + "\"";
  }
  resp += ">" + txt + "</button>";
  return resp;
}

String HtmlGenerator::submit(String txt) {
  return HtmlGenerator::inputBase("submit", "submit", txt);
}

String HtmlGenerator::hidden(String name, String value) {
  return HtmlGenerator::inputBase("hidden", name, value);
}

String HtmlGenerator::checkbox(String txt, String name, bool checked) {
  String resp = "<input type=\"checkbox\" name=\"" + name + "\"";
  if (checked) resp += " checked";
  resp += " value=\"1\">";
  resp += HtmlGenerator::labelBase(txt, name);
  return resp;
  
}

String HtmlGenerator::divStart(String additionalStyle) {
  return "<div" + (additionalStyle.length()>0?String(" style=\"" + additionalStyle + "\""):String("")) + String(">");
}

String HtmlGenerator::divEnd() {
  return "</div>";
}

String HtmlGenerator::p(String txt) {
  return "<p>" + txt + "</p>";
}

String HtmlGenerator::a(String txt, String link) {
  return "<a href=\"" + link + "\">" + txt + "</a>";
}

String HtmlGenerator::listStart() {
  return "<ul>";
}

String HtmlGenerator::listItem(String txt) {
  return "<li>" + txt + "</li>";
}

String HtmlGenerator::listEnd() {
  return "</ul>";
}

String HtmlGenerator::footer() {
  return "</body></html>";
}



