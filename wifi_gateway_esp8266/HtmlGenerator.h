#ifndef htmlgenerator_h
#define htmlgenerator_h
#include <Arduino.h>
#include <String.h>

class HtmlGenerator {
	private:
		static String inputBase(String type, String name, String val="");
		static String labelBase(String txt, String name);
	public:
		static String header(String title, String enc = "UTF-8");
		static String h(int level, String txt);
		static String formStart(String meth, String action);
		static String formEnd();
		static String textinput(String label, String name, String val="");
		static String passwordinput(String label, String name);
		static String button(String txt, String name, String type="button", String value="");
		static String submit(String txt);
		static String hidden(String name, String value);
    static String checkbox(String txt, String name, bool checked=false);
		static String divStart(String additionalStyle = "");
		static String divEnd();
		static String p(String txt);
    static String a(String txt, String link);
    static String listStart();
    static String listItem(String txt);
    static String listEnd();
		static String footer();
	
};

#endif
