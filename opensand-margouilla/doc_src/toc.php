
<script language="JavaScript">
<!-- 
var js_ok = false;
js_but = new Array();
if ( navigator.appName.substring(0,9) == "Microsoft" &&
parseInt(navigator.appVersion) >= 4 ) js_ok = true;
if ( navigator.appName.substring(0.8) == "Netscape" &&
parseInt(navigator.appVersion) >= 3 ) js_ok = true;

function js_button(pic, desc) {
  if (js_ok) {
    this.pic = new Image();
    this.pic.src = pic;
    this.pic_active = new Image();
    this.pic_active.src = pic.substring(0, pic.length -     4) + "-over.gif";
    this.text = desc;
    }
  }

function js_moveover(id) {
	if (js_ok) {
		document[id].src = js_but[id].pic_active.src;
		window.status = js_but[id].text;
	}
}

function js_moveaway(id) {
	if (js_ok) {
		document[id].src = js_but[id].pic.src;
		window.status = "";
	}
}

// -->
</script>


<?php

$count=1;

function add_menu_entry($i_is_sub_button, $i_url, $i_icon_name, $i_title) {
	global $count;
	$count +=1;
	echo "<img src=\"icon/pad.gif\" width=\"28\" height=\"1\" alt=\"\">";
	if ($i_is_sub_button==1) {
	  echo "<script language=\"JavaScript\">";
	  echo "<!--\n top.js_but[\"jsb_tube_subbutton".$count."\"] = ";
	  echo "new top.js_button(\"icon/tube-subbutton.gif\", \"".$i_url."\");";
	  echo "\n// --></script>";
	  echo "<a href=\"".$i_url."\" target=\"_top\" ";
	  echo "onmouseover=\"top.js_moveover('jsb_tube_subbutton".$count."'); return true; \" ";
	  echo "onmouseout=\"top.js_moveaway('jsb_tube_subbutton".$count."')\">";
	  echo "<img src=\"icon/tube-subbutton.gif\" width=\"46\" height=\"24\"  border=\"0\" alt=\"--&gt;\" name=\"jsb_tube_subbutton".$count."\">";
	  echo "<img src=\"icon/".$i_icon_name."\" width=\"125\" height=\"24\"  border=\"0\" alt=\"".$i_title."\"></a><br>";
	  echo "\n";
	} else {
	  echo "<script language=\"JavaScript\">";
	  echo "<!--\n top.js_but[\"jsb_tube_button".$count."\"] = ";
	  echo "new top.js_button(\"icon/tube-button.gif\", \"".$i_url."\");";
	  echo "\n// --></script>";
	  echo "<a href=\"".$i_url."\" target=\"_top\" ";
	  echo "onmouseover=\"top.js_moveover('jsb_tube_button".$count."'); return true; \" ";
	  echo "onmouseout=\"top.js_moveaway('jsb_tube_button".$count."')\">";
	  echo "<img src=\"icon/tube-button.gif\" width=\"46\" height=\"30\"  border=\"0\" alt=\"--&gt;\" name=\"jsb_tube_button".$count."\">";
	  echo "<img src=\"icon/".$i_icon_name."\" width=\"125\" height=\"30\"  border=\"0\" alt=\"".$i_title."\"></a><br>";
	  echo "\n";
	}
}

?>


<img src="icon/pad.gif" width="210" height="1"><br><img src="icon/tube-top.gif" width="74" height="23"  border="0"><br>
<?php
add_menu_entry(0, "about.htm", "about_margouilla_text.png", "About Margouilla");
add_menu_entry(1, "components.htm", "components_text.png", "Components & Tools");
add_menu_entry(1, "license.htm", "license_text.png",    "License");
add_menu_entry(0, "documentation.htm", "documentation_text.png", "Documentation");
add_menu_entry(1, "design_runtime.htm", "runtime_design_text.png", "Runtime design");
add_menu_entry(1, "tutorial_new_bloc.htm", "tutorial_new_bloc_text.png", "Tutorial: Coding a new bloc");
add_menu_entry(1, "exemple_msg_bloc.htm", "exemple_msg_bloc_text.png", "Exemple: Message exchanges between two blocs");
add_menu_entry(0, "download.htm", "download_text.png", "Download");
add_menu_entry(0, "mailing_list.htm", "mailing_list_text.png", "Mailing list");
add_menu_entry(0, "contact.htm", "contact_text.png", "Contact");
?>	

<img src="icon/tube-elem.gif" alt="" width="74" height="20"><br>
<img src="icon/tube-up.gif" width="93" height="21"  border="0" alt="[ Up ]"><br>
<img src="icon/tube-prev.gif" width="34" height="20"  border="0" alt="[Prev]"><img src="icon/tube-home.gif" width="20" height="20"  border="0" alt="[Home]"><img src="icon/tube-next.gif" width="39" height="20"  border="0" alt="[Next]" name="jsb_tube_next"></a><br>
<img src="icon/tube-mail.gif" width="93" height="34"  border="0" alt="[Mail]" name="jsb_tube_mail"></a>






