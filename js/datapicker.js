$(document).ready(function() {
    $('#embeddingDatePicker')
        .datepicker({
            format: 'mm/dd/yyyy'
        })
        .on('changeDate', function(e) {
            // Set the value for the date input
            $("#selectedDate").val($("#embeddingDatePicker").datepicker('getFormattedDate'));

            // Revalidate it
            $('#eventForm').formValidation('revalidateField', 'selectedDate');
        });

    $('#eventForm').formValidation({
        framework: 'bootstrap',
        icon: {
            valid: 'glyphicon glyphicon-ok',
            invalid: 'glyphicon glyphicon-remove',
            validating: 'glyphicon glyphicon-refresh'
        },
        fields: {
            name: {
                validators: {
                    notEmpty: {
                        message: 'The name is required'
                    }
                }
            },
            selectedDate: {
                // The hidden input will not be ignored
                excluded: false,
                validators: {
                    notEmpty: {
                        message: 'The date is required'
                    },
                    date: {
                        format: 'MM/DD/YYYY',
                        message: 'The date is not a valid'
                    }
                }
            }
        }
    });
});

