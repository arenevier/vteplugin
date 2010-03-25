vteplugin
=========

What is vteplugin ?
-------------------
vteplugin implements a terminal emulator in a browser plugins. That means you
can use command line in a terminal **inside a browser tab**.

But what is it useful for?
--------------------------
First, vteplugin was useful to me to learn more about how plugins work.
vteplugin is also useful if you spend most of your times in your browser, and
would have like your terminals rearranged as tabs to decrease your number of
windows opened. Then, it should be possible to hack vteplugin to have external
links (such as mailto:, irc:, ...) open directly in your console-based
application (mutt, irssi, ...), but in a browser tab. Then in the future, that
will may be improve your workflow. But currently, vteplugin does not do that.

How does it work technically?
-----------------------------
vteplugin is a npapi plugin that uses vte library to display terminal emulator.
For security reasons, websites are prevented to call this plugin, vteplugin
will only launch when calling from local file, or possibly from chrome protocol
in gecko browsers (in case you want to use vteplugin from an extension).

How do I install vteplugin?
---------------------------
run  
`$ ./waf configure && ./waf build`  
then, as root, run  
`# ./waf install`  
then, restart your browser, and open vteplugin.html document. This document
should display a usable terminal.

What are the supported platforms/browsers?
------------------------------------------
vteplugin has been developed and tested on linux. It should work unices that
support libvte. Please let me known if you encounter difficulties installing or
running it.
Gecko browsers are supported.
Webkit-gtk browsers are supported
Webkit-qt browsers status is unkwnown (let me known if you tried)
opera is **not** supported.

What is the license of vteplugin?
---------------------------------
npapi headers and boilerplate files come from mozilla tree. They are
tri-licensed under the MLP, GPL and LGPL
vteplugin.c, the actual plugin code is licensed under the WTFPL
see COPYING.TXT for more details

[arno](arno@renevier.net)
