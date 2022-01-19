//jQuery to collapse the navbar on scroll
$(window).scroll(function() {
    if ($(".navbar").offset().top > 50) {
        $(".navbar-fixed-top").addClass("top-nav-collapse");
		$(".logo").addClass("logo-collapse");
		$(".dropdown-menu").addClass("dropdown-collapse");
	
		//document.getElementById("logo").animate({height:'45px', width: '81px'});
    } else {
        $(".navbar-fixed-top").removeClass("top-nav-collapse");
		$(".logo").removeClass("logo-collapse");
		$(".dropdown-menu").removeClass("dropdown-collapse");
	
		//document.getElementById("logo").animate({height:'65px', width: '117px'});
    }
});


