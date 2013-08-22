Drupal.behaviors.mywebform = function() {
	var jscal;
	if(typeof Calendar != 'undefined') {
		jscal = Calendar.setup({onSelect: function(jscal) {jscal.hide()}, dateFormat : "%d.%m.%Y"});
		$('input.date').each(function () {
			var id = $(this).attr('id');
			if(id) {
				jscal.manageFields(id, id, "%d.%m.%Y");
			}
		});
	}
}

