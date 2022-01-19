from pathlib import Path


HEADER = """
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">

    <title>OpenSAND</title>

    <!-- JQuery Core CSS -->
	<link href="css/jquery-ui.css" rel="stylesheet">
	<link href="css/jquery-ui.min.css" rel="stylesheet">
	<link href="css/jquery-ui.theme.css" rel="stylesheet">
	<link href="css/jquery-ui.theme.min.css" rel="stylesheet">
	<link href="css/jquery-ui.structure.css" rel="stylesheet">
	<link href="css/jquery-ui.structure.min.css" rel="stylesheet">

    <!-- Bootstrap Core CSS -->
    <link href="css/bootstrap.min.css" rel="stylesheet">

    <!-- Custom CSS -->
    <link href="css/opensand.css" rel="stylesheet">
    <link href="css/scrolling-nav.css" rel="stylesheet">
    <link rel="icon" type="image/png" href="img/opensand-manager.png" />

    <!-- Custom Fonts -->
    <link href="font-awesome/css/font-awesome.min.css" rel="stylesheet" type="text/css">

    <!-- HTML5 Shim and Respond.js IE8 support of HTML5 elements and media queries -->
    <!-- WARNING: Respond.js doesn't work if you view the page via file:// -->
    <!--[if lt IE 9]>
      <script src="https://oss.maxcdn.com/libs/html5shiv/3.7.0/html5shiv.js" defer></script>
      <script src="https://oss.maxcdn.com/libs/respond.js/1.4.2/respond.min.js" defer></script>
    <![endif]-->

    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js" defer></script>

    <!-- jQuery -->
    <script src="js/jquery.js" defer></script>

    <!-- Bootstrap Core JavaScript -->
    <script src="js/bootstrap.min.js" defer></script>

    <!-- Scrolling Nav JavaScript -->
	<script src="js/jquery/jquery.js" defer></script>
	<script src="js/jquery/jquery-ui.js" defer></script>
	<script src="js/jquery/jquery-ui.min.js" defer></script>
    <script src="js/jquery.easing.min.js" defer></script>
    <script src="js/scrolling-nav.js" defer></script>
	<script src="js/jqBootstrapValidation.js" defer></script>
  </head>


  <body>
    <!-- Navigation -->
    <nav class="navbar navbar-inverse navbar-fixed-top navbar-custom" role="navigation">
      <div class="container">
        <!-- Brand and toggle get grouped for better mobile display -->
        <div class="navbar-header">
          <button type="button" class="navbar-toggle" data-toggle="collapse" data-target="#bs-example-navbar-collapse-1">
            <span class="sr-only">Toggle navigation</span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
          </button>
          <!-- <a class="navbar-brand page-scroll" href="index.html"> <img id="logo" src="img/logo2.png" alt=""/></a>-->
          <div id="imgLogo">
            <img class="logo" src="img/logo2.png" onclick="document.location.href = 'index.html'" alt=""></img>
          </div>
        </div>

        <!-- Collect the nav links, forms, and other content for toggling -->
        <div class="collapse navbar-collapse" id="bs-example-navbar-collapse-1">
        <ul class="nav navbar-nav navbar-right">
          <li class="dropdown page-scroll{}">
            <a href="#" class="dropdown-toggle" data-toggle="dropdown"> <p class="text-custom"> About <b class="caret"></b> </p> </a>
            <ul class="dropdown-menu">
              <li>
                <a href="overview.html" style="font-size:100%">Overview</a>
              </li>
              <li>
                <a href="references.html" style="font-size:100%">References</a>
              </li>
              <li>
                <a href="commitee.html" style="font-size:100%">Steering commitee</a>
              </li>
            </ul>
          </li>
          <li class="dropdown page-scroll{}">
            <a href="#" class="dropdown-toggle" data-toggle="dropdown"> <p class="text-custom"> Get & Learn <b class="caret"></b> </p></a>
            <ul class="dropdown-menu">
              <li>
                <a href="get.html" style="font-size:100%">Installation</a>
              </li>
              <li>
                <a href="mail.html" style="font-size:100%">Mailing-lists</a>
              </li>
              <li>
                <a href="https://wiki.net4sat.org/doku.php?id=opensand:index" style="font-size:100%">Wiki</a>
              </li>
              <li>
                <a href="libraries.html" style="font-size:100%">Libraries</a>
              </li>
              <li>
                <a href="relatedtools.html" style="font-size:100%">Related Tools</a>
              </li>
            </ul>
          </li>
          <li class="dropdown page-scroll{}">
            <a href="#" class="dropdown-toggle" data-toggle="dropdown"> <p class="text-custom"> Contribute  <b class="caret"></b> </p> </a>
            <ul class="dropdown-menu">
              <li>
                <a href="contribution.html" style="font-size:100%">Contribution</a>
              </li>
              <li>
                <a href="bugtracker.html" style="font-size:100%">Bugtracker</a>
              </li>
              <li>
                <a href="https://forge.net4sat.org/opensand/opensand" style="font-size:100%">Forge OpenSAND</a>
              </li>
            </ul>
          </li>
          <li class="page-scroll{}">
            <a href="blog.html" ><p class="text-custom">Blog </p></a>
          </li>

          <li class="page-scroll">
            <a href="https://www.net4sat.org"> <img src="img/gitlab-logo-square.png"/> </a>
          </li>
        </ul>
        </div>
      </div>
    </nav>
"""
FOOTER = """
    <!-- Footer -->
    <footer>
      <div class="container">
        <div class="row">
          <div class="col-lg-12">
            <div id="licenses">
              <p style="font-size:90%">Contact <a href="mailto:admin _AT_ opensand _DOT_ org" title="OpenSAND contact">
              OpenSAND Steering Committee <i class="fa fa-envelope" aria-hidden="true"></i></a></p>
              <p style="font-size:90%">The text content of this website is published under the <a 
              href="http://creativecommons.org/licenses/by-nc-sa/3.0/" title="See license details"
              rel="license" hreflang="en">Creative Commons BY-NC-SA license</a> (except where otherwise
              stated). Some sections icons are derivative work of the GPL2 icons provided by the Gnome
              project.</p>
              <p style="font-size:90%">Logos and trademarks are the property of their respective owners. Any representation,
              reproduction and/or exploitation, whether partial or total, of trademarks and logos
              is prohibited without prior written permission from the owners.</p>
              <p style="font-size:90%"><a href="privacy_policy.html" title="Read the privacy policy">
              Privacy policy</a>.</p>
            </div>
          </div>
        </div>
      </div>
    </footer>
  </body>
</html>
"""

ACTIVE_PAGE = [
        ('overview.html', 'references.html', 'commitee.html',),
        ('get.html', 'mail.html', 'libraries.html', 'relatedtools.html',),
        ('contribution.html', 'bugtracker.html', 'agreement.html',),
        ('blog.html',),
]


def main(folder):
    for filepath in folder.glob('*.html'):
        filename = filepath.name
        active_page = (
                ' active' if filename in dropdown else ''
                for dropdown in ACTIVE_PAGE
        )
        with filepath.parent.parent.joinpath(filename).open('w') as f:
            print(HEADER.format(*active_page), file=f)
            with filepath.open() as content:
                print(content.read(), file=f)
            print(FOOTER, file=f)


if __name__ == '__main__':
    main(Path(__file__).resolve().parent)
