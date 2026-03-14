#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mud.h"

/*
 * Free an account structure from memory
 */
void free_account( ACCOUNT_DATA *account )
{
    int i;
    
    if ( !account ) 
        return;
        
    STRFREE( account->name );
    STRFREE( account->pwd );
    
    for ( i = 0; i < MAX_ACCOUNT_CHARS; i++ )
    {
        if ( account->character[i] )
            STRFREE( account->character[i] );
    }
    DISPOSE( account );
}

/*
 * Save an account to disk
 */
void save_account( ACCOUNT_DATA *account )
{
    FILE *fp;
    char strsave[MAX_INPUT_LENGTH];
    int i;

    if( !account || !account->name ) 
        return;

    snprintf( strsave, MAX_INPUT_LENGTH, "%s%s", ACCOUNT_DIR, capitalize( account->name ) );

    if( ( fp = fopen( strsave, "w" ) ) == NULL )
    {
        bug( "%s: fopen", __func__ );
        perror( strsave );
    }
    else
    {
        fprintf( fp, "Name     %s~\n", account->name );
        if ( account->pwd )
            fprintf( fp, "Password %s~\n", account->pwd );
            
        for ( i = 0; i < MAX_ACCOUNT_CHARS; i++ )
        {
            if ( account->character[i] )
                fprintf( fp, "Char     %s~\n", account->character[i] );
        }
        fprintf( fp, "End\n" );
        FCLOSE( fp );
    }
}

/*
 * Load an account from disk
 */
ACCOUNT_DATA *load_account( const char *name )
{
    FILE *fp;
    char strsave[MAX_INPUT_LENGTH];
    ACCOUNT_DATA *account;
    char *word;
    bool fMatch;
    int i = 0;

    snprintf( strsave, MAX_INPUT_LENGTH, "%s%s", ACCOUNT_DIR, capitalize( name ) );

    if( ( fp = fopen( strsave, "r" ) ) == NULL )
        return NULL;

    CREATE( account, ACCOUNT_DATA, 1 );
    account->name = NULL;
    account->pwd = NULL;
    for( i = 0; i < MAX_ACCOUNT_CHARS; i++ )
        account->character[i] = NULL;

    i = 0;

    for( ;; )
    {
        word = feof( fp ) ? (char*)"End" : fread_word( fp );
        fMatch = FALSE;

        switch( UPPER(word[0]) )
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
                
            case 'C':
                if ( !str_cmp( word, "Char" ) )
                {
                    if ( i < MAX_ACCOUNT_CHARS )
                    {
                        account->character[i] = fread_string( fp );
                        i++;
                    }
                    else
                        fread_flagstring( fp ); /* discard if over max */
                    fMatch = TRUE;
                }
                break;
                
            case 'E':
                if ( !str_cmp( word, "End" ) )
                {
                    FCLOSE( fp );
                    return account;
                }
                break;
                
            case 'N':
                KEY( "Name", account->name, fread_string( fp ) );
                break;
                
            case 'P':
                KEY( "Password", account->pwd, fread_string( fp ) );
                break;
        }
        if( !fMatch )
        {
            bug( "Load_account: no match: %s", word );
            fread_to_eol( fp );
        }
    }
    FCLOSE( fp );
    return account;
}
