<?php
////////////////////////////////////////////////////////////////
// Following Variables must be defined when including "core.php"
// $Header: File name of the header picture.
// $Body: File name of the HTML file containing the body text.
// $Frame: Set to 1 of you want to include the core in a frame.
//
?>

<html>
<head>
<title>Document sans-titre</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
</head>

<body bgcolor="#FFFFFF">
<table width="100%" border="0">
  <tr>
    <td width="12%" valign="top">
      <table width="100%" border="0">
        <tr valign="top"> 
          <td><img src="icon/logo_bleusimple_small2.jpg"></td>
        </tr>
        <tr valign="top"> 
          <td>
            <p>	
			<?php include("toc.php"); ?> 
			</p>
            <p>&nbsp;</p>
            <p>&nbsp;</p>
            <p>&nbsp;</p>
            <p>&nbsp;</p>
          </td>
        </tr>
      </table>
    </td>
    <td width="88%">
      <table width="100%" border="0">
<?php include("content_header_page.htm"); ?> 
        <tr valign="top"> 
          <td>
            <hr noshade>
          </td>
        </tr>
        <tr valign="top"> 
          <td>
            <p>
			<?php include($Body); ?> 
			</p>
          </td>
        </tr>
      </table>
    </td>
  </tr>
</table>
<p>
<hr noshade>
<?php include("content_bas_page.htm"); ?> 
</p>
</body>
</html>

