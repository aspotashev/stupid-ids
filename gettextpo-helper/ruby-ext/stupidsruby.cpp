
#include <ruby.h>

#include "module.h"
#include "globals.h"
#include "gitloader.h"
#include "translationcontent.h"
#include "idmapdb.h"
#include "message.h"


extern "C" {

// Function called at module loading
void Init_stupidsruby()
{
	GettextpoHelper = rb_define_module("GettextpoHelper");
	init_globals();
	init_GitLoader();
	init_TranslationContent();
	init_IdMapDb();
	init_Message();
}

}

