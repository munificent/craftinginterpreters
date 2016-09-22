$(document).ready(function() {
  $(".dismiss").click(function() {
    $(".sign-up").hide();
  });

  $("#expand-nav").click(function() {
    $(".expandable").toggleClass("shown");
  });

  $(window).scroll(function() {
    var nav = $("nav.floating");
    console.log($(window).scrollTop());
    if ($(window).scrollTop() > 84) {
      nav.addClass("pinned");
    } else {
      nav.removeClass("pinned");
    }
  });

  $(window).resize(refreshAsides);

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
    var span = $("span[name='" + name + "']");
    if (span == null) {
      window.console.log("Could not find span for '" + name + "'");
      return;
    }

    // Vertically position the aside next to the span it annotates.
    aside.offset({top: span.position().top - 6});
  });
}