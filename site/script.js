$(document).ready(function() {
  $(".dismiss").click(function() {
    $(".sign-up").hide();
    refreshAsides();

    // Don't show it again for a while.
    // From: http://www.quirksmode.org/js/cookies.html.
    var date = new Date();
    var days = 10;
    date.setTime(date.getTime() + (10 * 24 * 60 * 60 * 1000));
    document.cookie =
        "hidesignup=true; expires=" + date.toGMTString() + "; path=/";
  });

  $("#expand-nav").click(function() {
    $(".expandable").toggleClass("shown");
  });

  $(window).scroll(function() {
    var nav = $("nav.floating");
    if ($(window).scrollTop() > 84) {
      nav.addClass("pinned");
    } else {
      nav.removeClass("pinned");
    }
  });

  $(window).resize(refreshAsides);

  // Hide the sign-up box if the user has already dismissed it.
  if (document.cookie.indexOf("hidesignup") != -1) {
    $(".sign-up").hide();
  }

  // Since we may not have the height correct for the images, adjust the asides
  // too when an image is loaded.
  $("img").load(function() {
    refreshAsides();
  });

  // On the off chance the browser supports the new font loader API, use it.
  if (document.fontloader) {
    document.fontloader.notifyWhenFontsReady(function() {
      refreshAsides();
    });
  }

  // Lame. Just do another refresh after a second when the font is *probably*
  // loaded to hack around the fact that the metrics changed a bit.
  window.setTimeout(refreshAsides, 200);

  refreshAsides();
});

function refreshAsides() {
  $("aside").each(function() {
    var aside = $(this);

    // If the asides are inline, clear their position.
    if ($(document).width() <= 48 * 20) {
      aside.css('top', 'auto');
      return;
    }

    // Find the span the aside should be anchored next to.
    var name = aside.attr("name");
    if (name == null) {
      window.console.log("No name for aside:");
      window.console.log(aside.context);
      return;
    }

    var span = $("span[name='" + name + "']");
    if (span == null) {
      window.console.log("Could not find span for '" + name + "'");
      return;
    }

    // Vertically position the aside next to the span it annotates.
    aside.offset({top: span.position().top - 6});
  });
}